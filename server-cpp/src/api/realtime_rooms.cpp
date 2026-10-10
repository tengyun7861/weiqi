#include "weiqi/api/http_common.hpp"
#include "weiqi/api/realtime_server.hpp"
#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
void RealtimeServer::rooms(crow::websocket::connection &connection, const ClientSession &session,
                           const std::string &type, const json &payload, const std::string &request_id) {
    if (type == "room.leave") {
        if (!session.room_code.empty())
            games_.leave_room(session.user_id, session.room_code);
        std::lock_guard lock(clients_mutex_);
        clients_[&connection].room_code.clear();
        return;
    }
    const auto code = payload.at("roomCode").get<std::string>();
    const bool spectating = type == "spectate.join";
    const auto state = games_.join_room(session.user_id, code, spectating);
    {
        std::lock_guard lock(clients_mutex_);
        clients_[&connection].room_code = code;
        clients_[&connection].spectating = spectating;
    }
    connection.send_text(json{
        {"type", "room.snapshot"},
        {"requestId", request_id},
        {"payload", snapshot_json(state)},
        {"sequence",
         state.sequence}}.dump());
    const auto messages = games_.chat_history(session.user_id, code);
    json history = json::array();
    for (const auto &line : messages)
        history.push_back(
            {{"nickname", line.nickname}, {"content", line.content}, {"createdAt", line.created_at}});
    connection.send_text(json{
        {"type", "chat.history"}, {"payload", {{"roomCode", code}, {"messages", history}}}, {"sequence", 0}}
                             .dump());
    // Legacy transport clients still receive marked historical events.
    for (const auto &line : messages)
        connection.send_text(json{
            {"type", "chat.message"},
            {"payload", {{"nickname", line.nickname}, {"message", line.content}, {"historical", true}}},
            {"sequence",
             state.sequence}}.dump());
    if (state.ai_white && !state.finished && state.current_player == Stone::white)
        ai_.schedule(code);
}
} // namespace weiqi::api
