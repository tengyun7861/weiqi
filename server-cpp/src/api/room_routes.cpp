#include "weiqi/api/http_common.hpp"
#include "weiqi/api/routes.hpp"
#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
void register_room_routes(crow::SimpleApp &app, AuthService &auth, GameService &games) {
    CROW_ROUTE(app, "/api/v1/rooms").methods(crow::HTTPMethod::Get)([&](const crow::request &request) {
        if (!user_for(request, auth))
            return unauthorized();
        json rooms = json::array();
        for (const auto &room : games.public_rooms())
            rooms.push_back(
                {{"roomCode", room.code}, {"boardSize", room.board_size}, {"status", room.status}});
        return reply(true, std::move(rooms));
    });
    CROW_ROUTE(app, "/api/v1/rooms").methods(crow::HTTPMethod::Post)([&](const crow::request &request) {
        const auto user = user_for(request, auth);
        if (!user)
            return unauthorized();
        try {
            const auto body = json::parse(request.body);
            const int main = body.value("mainTimeSeconds", 600), byo = body.value("byoYomiSeconds", 30);
            if (main < 0 || main > 7200 || byo < 1 || byo > 120)
                throw std::invalid_argument("计时配置超出范围");
            const auto code = games.create_room(user->id, body.value("boardSize", 19),
                                                body.value("isPublic", true), main, byo);
            return reply(true, {{"roomCode", code}}, {}, 201);
        } catch (const std::exception &error) {
            return reply(false, {}, error.what(), 400);
        }
    });
    CROW_ROUTE(app, "/api/v1/rooms/<string>/join")
        .methods(crow::HTTPMethod::Post)([&](const crow::request &request, const std::string &code) {
            const auto user = user_for(request, auth);
            if (!user)
                return unauthorized();
            try {
                return reply(true, snapshot_json(games.join_room(user->id, code, false)));
            } catch (const std::exception &error) {
                return reply(false, {}, error.what(), 400);
            }
        });
    CROW_ROUTE(app, "/api/v1/rooms/<string>/leave")
        .methods(crow::HTTPMethod::Post)([&](const crow::request &request, const std::string &code) {
            const auto user = user_for(request, auth);
            if (!user)
                return unauthorized();
            try {
                games.leave_room(user->id, code);
                return reply(true, {{"message", "已离开房间，座位会为重连保留"}});
            } catch (const std::exception &error) {
                return reply(false, {}, error.what(), 400);
            }
        });
}
} // namespace weiqi::api
