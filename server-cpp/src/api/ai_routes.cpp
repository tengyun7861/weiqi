#include "weiqi/api/http_common.hpp"
#include "weiqi/api/routes.hpp"
#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
void register_ai_routes(crow::SimpleApp &app, AuthService &auth, GameService &games, KataGoGtp &katago) {
    CROW_ROUTE(app, "/api/v1/katago")([&] {
        const auto status = katago.status();
        return reply(
            true, {{"available", katago.available() && status == "KataGo GTP 已就绪"}, {"reason", status}});
    });
    CROW_ROUTE(app, "/api/v1/ai/games").methods(crow::HTTPMethod::Post)([&](const crow::request &request) {
        const auto user = user_for(request, auth);
        if (!user)
            return unauthorized();
        try {
            const auto body = json::parse(request.body);
            if (!katago.available())
                return reply(false, {}, katago.status(), 503);
            const auto status = katago.status();
            if (status != "KataGo GTP 已就绪")
                return reply(false, {}, status, 503);
            const auto code = games.create_ai_room(user->id, body.value("boardSize", 19));
            return reply(true, {{"roomCode", code}}, {}, 201);
        } catch (const std::exception &error) {
            return reply(false, {}, error.what(), 400);
        }
    });
}
} // namespace weiqi::api
