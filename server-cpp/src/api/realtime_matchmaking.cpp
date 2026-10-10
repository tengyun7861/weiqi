#include "weiqi/api/http_common.hpp"
#include "weiqi/api/realtime_server.hpp"

namespace weiqi::api {
void RealtimeServer::matchmaking(crow::websocket::connection &connection, const ClientSession &session,
                                 const std::string &type, const json &payload,
                                 const std::string &request_id) {
    if (type == "match.cancel") {
        games_.cancel_matchmaking(session.user_id);
        return;
    }
    const auto matched = games_.join_matchmaking(session.user_id, payload.value("boardSize", 19));
    if (!matched)
        connection.send_text(json{{"type", "match.waiting"},
                                  {"requestId", request_id},
                                  {"payload", {{"message", "正在等待匹配对手"}}},
                                  {"sequence", 0}}
                                 .dump());
}
} // namespace weiqi::api
