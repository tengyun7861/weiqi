#pragma once
#include "weiqi/ai_worker.hpp"
#include "weiqi/auth.hpp"
#include <crow.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace weiqi::api {
struct ClientSession {
    int user_id{};
    std::string nickname, token, room_code;
    bool spectating{};
    int last_request_sequence{};
};

// Owns transport/session state; domain behavior is implemented in separate command files.
class RealtimeServer final {
  public:
    RealtimeServer(AuthService &auth, GameService &games, KataGoGtp &katago);
    ~RealtimeServer();
    void attach(crow::SimpleApp &app);

  private:
    void broadcast(const RealtimeEvent &event);
    void close(crow::websocket::connection &connection);
    void receive(crow::websocket::connection &connection, const std::string &message, bool binary);
    void rooms(crow::websocket::connection &connection, const ClientSession &session, const std::string &type,
               const nlohmann::json &payload, const std::string &request_id);
    void matchmaking(crow::websocket::connection &connection, const ClientSession &session,
                     const std::string &type, const nlohmann::json &payload, const std::string &request_id);
    void gameplay(const ClientSession &session, const std::string &type, const nlohmann::json &payload);
    void chat(const ClientSession &session, const nlohmann::json &payload);
    AuthService &auth_;
    GameService &games_;
    AiWorker ai_;
    std::mutex clients_mutex_;
    std::unordered_map<crow::websocket::connection *, ClientSession> clients_;
};
} // namespace weiqi::api
