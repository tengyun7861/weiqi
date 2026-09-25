#include "weiqi/auth.hpp"
#include "weiqi/database.hpp"
#include "weiqi/game_service.hpp"

#include <winsock2.h>
#include <windows.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {
class WsClient final {
public:
    ~WsClient() { if (socket_ != INVALID_SOCKET) closesocket(socket_); }

    void connect_to(unsigned short port) {
        sockaddr_in address{}; address.sin_family=AF_INET; address.sin_port=htons(port); address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        for(int attempt=0;attempt<100;++attempt) { socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); assert(socket_ != INVALID_SOCKET); if(::connect(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0) break; closesocket(socket_); socket_=INVALID_SOCKET; std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        assert(socket_ != INVALID_SOCKET);
        const std::string request="GET /ws HTTP/1.1\r\nHost: 127.0.0.1:"+std::to_string(port)+"\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: d2VpcWktZTI2LXRlc3Q=\r\nSec-WebSocket-Version: 13\r\n\r\n";
        assert(send(socket_,request.data(),static_cast<int>(request.size()),0)==static_cast<int>(request.size()));
        while (buffer_.find("\r\n\r\n") == std::string::npos) receive_more();
        assert(buffer_.starts_with("HTTP/1.1 101")); buffer_.erase(0,buffer_.find("\r\n\r\n")+4);
    }

    void send_json(const std::string& json) {
        assert(json.size()<65536); std::string frame; frame.push_back(static_cast<char>(0x81)); if(json.size()<126) frame.push_back(static_cast<char>(0x80|json.size())); else { frame.push_back(static_cast<char>(0x80|126)); frame.push_back(static_cast<char>((json.size()>>8)&0xff)); frame.push_back(static_cast<char>(json.size()&0xff)); }
        constexpr unsigned char mask[]{0x11,0x22,0x33,0x44}; for (const auto value:mask) frame.push_back(static_cast<char>(value));
        for(size_t i=0;i<json.size();++i) frame.push_back(static_cast<char>(json[i]^mask[i%4]));
        assert(send(socket_,frame.data(),static_cast<int>(frame.size()),0)==static_cast<int>(frame.size()));
    }

    std::string until(const std::string& needle) {
        const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
        while(std::chrono::steady_clock::now()<deadline) { const auto frame=next_frame(); if(frame.find(needle)!=std::string::npos) return frame; }
        std::cerr<<"未收到事件："<<needle<<'\n'; assert(false); return {};
    }

private:
    void receive_more() {
        fd_set readable; FD_ZERO(&readable); FD_SET(socket_,&readable); timeval timeout{1,0}; assert(select(0,&readable,nullptr,nullptr,&timeout)>0);
        char incoming[4096]{}; const auto count=recv(socket_,incoming,sizeof(incoming),0); assert(count>0); buffer_.append(incoming,count);
    }
    std::string next_frame() {
        while(buffer_.size()<2) receive_more(); const auto length=static_cast<unsigned char>(buffer_[1])&0x7f; assert((static_cast<unsigned char>(buffer_[1])&0x80)==0); size_t header=2; size_t size=length;
        if(length==126) { while(buffer_.size()<4) receive_more(); header=4; size=(static_cast<unsigned char>(buffer_[2])<<8)|static_cast<unsigned char>(buffer_[3]); }
        assert(length!=127); while(buffer_.size()<header+size) receive_more(); const auto opcode=static_cast<unsigned char>(buffer_[0])&0x0f; const auto payload=buffer_.substr(header,size); buffer_.erase(0,header+size); if(opcode==0x9) return next_frame(); assert(opcode==0x1); return payload;
    }
    SOCKET socket_{INVALID_SOCKET}; std::string buffer_;
};

PROCESS_INFORMATION start_server(const char* path, const std::string& database, unsigned short port) {
    const std::string command="\""+std::string(path)+"\" \""+database+"\" "+std::to_string(port); std::vector<char> mutable_command(command.begin(),command.end()); mutable_command.push_back('\0');
    STARTUPINFOA startup{}; startup.cb=sizeof(startup); PROCESS_INFORMATION process{}; assert(CreateProcessA(path,mutable_command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process)); CloseHandle(process.hThread); return process;
}
void stop_server(PROCESS_INFORMATION process) { TerminateProcess(process.hProcess,0); WaitForSingleObject(process.hProcess,3000); CloseHandle(process.hProcess); }
}

int main(int argc, char* argv[]) {
    std::cerr<<"e2e start argc="<<argc<<'\n'; if(argc!=2) return 2; constexpr unsigned short port=18082; const auto database=(std::filesystem::temp_directory_path()/("weiqi_websocket_e2e_"+std::to_string(GetCurrentProcessId())+".sqlite3")).string();
    std::string black_token, white_token, spectator_token, room_code;
    {
        std::cerr<<"prepare database\n"; weiqi::Database database_handle(database); database_handle.migrate(); weiqi::AuthService auth(database_handle);
        auth.register_user("黑方", "black-player-password"); auth.register_user("白方", "white-player-password"); auth.register_user("观战者", "spectator-password");
        const auto black=auth.authenticate(auth.login("黑方","black-player-password")); const auto white=auth.authenticate(auth.login("白方","white-player-password")); assert(black&&white);
        black_token=auth.login("黑方","black-player-password"); white_token=auth.login("白方","white-player-password"); spectator_token=auth.login("观战者","spectator-password");
        weiqi::GameService games(database_handle); room_code=games.create_room(black->id,9,true,60,10);
    }
    std::cerr<<"start server\n"; WSADATA data{}; assert(WSAStartup(MAKEWORD(2,2),&data)==0); const auto process=start_server(argv[1],database,port);
    WsClient black,white,spectator,reconnected; std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cerr<<"black handshake\n"; black.connect_to(port); black.until("请先发送 auth"); black.send_json("{\"type\":\"auth\",\"requestId\":\"a1\",\"payload\":{\"token\":\""+black_token+"\"},\"sequence\":1}"); black.until("\"userId\"");
    std::cerr<<"white handshake\n"; white.connect_to(port); white.until("请先发送 auth"); white.send_json("{\"type\":\"auth\",\"requestId\":\"b1\",\"payload\":{\"token\":\""+white_token+"\"},\"sequence\":1}"); white.until("\"userId\"");
    std::cerr<<"room join\n"; black.send_json("{\"type\":\"room.join\",\"requestId\":\"a2\",\"payload\":{\"roomCode\":\""+room_code+"\"},\"sequence\":2}"); black.until("room.snapshot");
    white.send_json("{\"type\":\"room.join\",\"requestId\":\"b2\",\"payload\":{\"roomCode\":\""+room_code+"\"},\"sequence\":2}"); white.until("room.snapshot");
    std::cerr<<"move\n"; black.send_json("{\"type\":\"game.move\",\"requestId\":\"a3\",\"payload\":{\"x\":0,\"y\":0},\"sequence\":3}"); black.until("game.updated"); white.until("game.updated");
    white.send_json("{\"type\":\"chat.send\",\"requestId\":\"b3\",\"payload\":{\"content\":\"联机聊天\"},\"sequence\":3}"); black.until("联机聊天"); white.until("联机聊天");
    std::cerr<<"spectator\n"; spectator.connect_to(port); spectator.until("请先发送 auth"); spectator.send_json("{\"type\":\"auth\",\"requestId\":\"c1\",\"payload\":{\"token\":\""+spectator_token+"\"},\"sequence\":1}"); spectator.until("\"userId\""); spectator.send_json("{\"type\":\"spectate.join\",\"requestId\":\"c2\",\"payload\":{\"roomCode\":\""+room_code+"\"},\"sequence\":2}"); spectator.until("room.snapshot"); spectator.until("联机聊天"); spectator.send_json("{\"type\":\"game.move\",\"requestId\":\"c3\",\"payload\":{\"x\":1,\"y\":0},\"sequence\":3}"); spectator.until("当前无权落子");
    // New connection with the same token restores the player's seat and authoritative board snapshot.
    std::cerr<<"reconnect\n"; reconnected.connect_to(port); reconnected.until("请先发送 auth"); reconnected.send_json("{\"type\":\"auth\",\"requestId\":\"r1\",\"payload\":{\"token\":\""+black_token+"\"},\"sequence\":1}"); reconnected.until("\"userId\""); reconnected.send_json("{\"type\":\"room.join\",\"requestId\":\"r2\",\"payload\":{\"roomCode\":\""+room_code+"\"},\"sequence\":2}"); assert(reconnected.until("room.snapshot").find("\"stones\":[1")!=std::string::npos);
    stop_server(process); WSACleanup(); std::filesystem::remove(database); std::filesystem::remove(database+"-wal"); std::filesystem::remove(database+"-shm");
}
