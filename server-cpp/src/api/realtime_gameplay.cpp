#include "weiqi/api/realtime_server.hpp"

namespace weiqi::api {
void RealtimeServer::gameplay(const ClientSession &session, const std::string &type,
                              const nlohmann::json &payload) {
    if (session.spectating || session.room_code.empty())
        throw std::invalid_argument("当前无权落子：观战者不能操作对局");
    if (type == "game.move")
        (void)games_.move(session.user_id, session.room_code, payload.at("x"), payload.at("y"));
    else if (type == "game.pass")
        (void)games_.pass(session.user_id, session.room_code);
    else if (type == "game.resign")
        (void)games_.resign(session.user_id, session.room_code);
    else if (games_.room_state(session.room_code).black_user != session.user_id)
        throw std::invalid_argument("无权重试该人机对局");
    if (games_.ai_turn(session.room_code))
        ai_.schedule(session.room_code);
}
} // namespace weiqi::api
