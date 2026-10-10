#include "weiqi/api/realtime_server.hpp"

namespace weiqi::api {
void RealtimeServer::chat(const ClientSession &session, const nlohmann::json &payload) {
    if (session.room_code.empty())
        throw std::invalid_argument("请先进入房间");
    games_.send_chat(session.user_id, session.nickname, session.room_code, payload.at("content"));
}
} // namespace weiqi::api
