#include "weiqi/api/realtime_server.hpp"
#include "weiqi/api/http_common.hpp"
#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
RealtimeServer::RealtimeServer(AuthService &auth, GameService &games, KataGoGtp &katago)
    : auth_(auth), games_(games), ai_(games, katago) {
    games_.set_event_listener([this](const RealtimeEvent &event) { broadcast(event); });
}
RealtimeServer::~RealtimeServer() {
    games_.set_event_listener({});
}

void RealtimeServer::broadcast(const RealtimeEvent &event) {
    const auto wire = event_json(event).dump();
    std::lock_guard lock(clients_mutex_);
    for (auto &[connection, session] : clients_) {
        const bool directed = event.user_id && *event.user_id == session.user_id;
        const bool in_room = !event.room_code.empty() && event.room_code == session.room_code;
        if (!directed && !in_room)
            continue;
        if (directed && (event.type == "match.found" || event.type == "room.snapshot"))
            session.room_code = event.room_code;
        if (event.type == "match.found" && directed)
            session.spectating = false;
        connection->send_text(wire);
    }
}

void RealtimeServer::close(crow::websocket::connection &connection) {
    ClientSession closing;
    bool known{}, still_connected{};
    {
        std::lock_guard lock(clients_mutex_);
        const auto it = clients_.find(&connection);
        if (it != clients_.end()) {
            closing = it->second;
            known = true;
            clients_.erase(it);
        }
        for (const auto &[other, session] : clients_)
            if (session.user_id == closing.user_id)
                still_connected = true;
    }
    if (known && !still_connected) {
        games_.cancel_matchmaking(closing.user_id);
        if (!closing.room_code.empty())
            games_.leave_room(closing.user_id, closing.room_code);
    }
}

void RealtimeServer::receive(crow::websocket::connection &connection, const std::string &message,
                             bool binary) {
    std::string request_id;
    try {
        if (binary)
            throw std::invalid_argument("不支持二进制消息");
        const auto incoming = json::parse(message);
        const auto type = incoming.value("type", "");
        request_id = incoming.value("requestId", "");
        if (type == "auth") {
            const auto token = incoming.at("payload").at("token").get<std::string>();
            const auto user = auth_.authenticate(token);
            if (!user)
                throw std::invalid_argument("登录已过期或无效");
            {
                std::lock_guard lock(clients_mutex_);
                clients_[&connection] = {user->id, user->nickname, token, {}, false, 0};
            }
            connection.send_text(json{{"type", "session.ready"},
                                      {"requestId", request_id},
                                      {"payload", {{"userId", user->id}, {"nickname", user->nickname}}},
                                      {"sequence", 1}}
                                     .dump());
            return;
        }
        ClientSession session;
        {
            std::lock_guard lock(clients_mutex_);
            const auto it = clients_.find(&connection);
            if (it == clients_.end())
                throw std::invalid_argument("请先使用 auth 鉴权");
            const int sequence = incoming.value("sequence", 0);
            if (sequence <= it->second.last_request_sequence)
                throw std::invalid_argument("实时请求序号无效或重复");
            it->second.last_request_sequence = sequence;
            session = it->second;
        }
        if (!auth_.authenticate(session.token))
            throw std::invalid_argument("登录已过期，请重新登录");
        const auto payload = incoming.value("payload", json::object());
        if (type == "match.join" || type == "match.cancel")
            matchmaking(connection, session, type, payload, request_id);
        else if (type == "room.join" || type == "spectate.join" || type == "room.leave")
            rooms(connection, session, type, payload, request_id);
        else if (type == "game.move" || type == "game.pass" || type == "game.resign" || type == "ai.retry")
            gameplay(session, type, payload);
        else if (type == "chat.send")
            chat(session, payload);
        else
            throw std::invalid_argument("未知实时事件：" + type);
        connection.send_text(json{
            {"type", "request.ok"}, {"requestId", request_id}, {"payload", json::object()}, {"sequence", 0}}
                                 .dump());
    } catch (const std::exception &error) {
        connection.send_text(json{{"type", "error"},
                                  {"requestId", request_id},
                                  {"payload", {{"message", error.what()}}},
                                  {"sequence", 0}}
                                 .dump());
    }
}

void RealtimeServer::attach(crow::SimpleApp &app) {
    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .max_payload(16384)
        .onaccept([](const crow::request &request, void **) {
            const auto origin = request.get_header_value("Origin"), host = request.get_header_value("Host");
            return origin.empty() || origin == "http://" + host || origin == "https://" + host;
        })
        .onopen([](crow::websocket::connection &connection) {
            connection.send_text(json{
                {"type", "session.ready"}, {"payload", {{"message", "请先发送 auth 事件"}}}, {"sequence", 0}}
                                     .dump());
        })
        .onclose([this](crow::websocket::connection &connection, const std::string &, uint16_t) {
            close(connection);
        })
        .onmessage([this](crow::websocket::connection &connection, const std::string &message, bool binary) {
            receive(connection, message, binary);
        });
}
} // namespace weiqi::api
