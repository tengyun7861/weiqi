#include <winsock2.h>
#include <windows.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>
#include <iostream>
#include <thread>
#include <vector>

int main(int argc, char* argv[]) {
    assert(argc == 2); const auto database=(std::filesystem::temp_directory_path()/"weiqi_api_smoke.sqlite3").string();
    const std::string command="\""+std::string(argv[1])+"\" \""+database+"\" 18081"; std::vector<char> mutable_command(command.begin(),command.end()); mutable_command.push_back('\0');
    STARTUPINFOA startup{}; startup.cb=sizeof(startup); PROCESS_INFORMATION process{}; assert(CreateProcessA(argv[1],mutable_command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process)); CloseHandle(process.hThread);
    WSADATA data{}; assert(WSAStartup(MAKEWORD(2,2),&data)==0); SOCKET socket=INVALID_SOCKET;
    for(int attempt=0;attempt<100 && socket==INVALID_SOCKET;++attempt){ DWORD exit_code{}; if(WaitForSingleObject(process.hProcess,0)==WAIT_OBJECT_0){ GetExitCodeProcess(process.hProcess,&exit_code); std::cerr<<"服务端提前退出，退出码="<<exit_code<<'\n'; break; } socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP); sockaddr_in address{}; address.sin_family=AF_INET; address.sin_port=htons(18081); address.sin_addr.s_addr=htonl(INADDR_LOOPBACK); if(connect(socket,reinterpret_cast<sockaddr*>(&address),sizeof(address))==SOCKET_ERROR){ const auto error=WSAGetLastError(); closesocket(socket); socket=INVALID_SOCKET; if(attempt==99) std::cerr<<"无法连接服务端，WSA 错误="<<error<<'\n'; std::this_thread::sleep_for(std::chrono::milliseconds(100)); } }
    std::string response;
    if(socket!=INVALID_SOCKET) { const char request[]="GET /api/v1/health HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n"; assert(send(socket,request,sizeof(request)-1,0)>0); DWORD receive_timeout=3000; setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&receive_timeout),sizeof(receive_timeout)); char buffer[2048]{}; for(;;){ const int received=recv(socket,buffer,sizeof(buffer),0); if(received<=0) break; response.append(buffer,received); if(response.find("\"ok\":true")!=std::string::npos) break; } closesocket(socket); }
    WSACleanup(); TerminateProcess(process.hProcess,0); WaitForSingleObject(process.hProcess,3000); CloseHandle(process.hProcess);
    assert(!response.empty()); assert(response.find("\"ok\":true")!=std::string::npos); std::filesystem::remove(database); std::filesystem::remove(database+"-wal"); std::filesystem::remove(database+"-shm");
}
