#include "weiqi/api/http_common.hpp"
#include "weiqi/api/routes.hpp"
#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
void register_replay_routes(crow::SimpleApp &app, AuthService &auth) {
    CROW_ROUTE(app, "/api/v1/replay/import")
        .methods(crow::HTTPMethod::Post)([&](const crow::request &request) {
            if (!user_for(request, auth))
                return unauthorized();
            try {
                if (request.body.size() > 3 * 1024 * 1024)
                    return reply(false, {}, "棋谱文件过大", 413);
                const auto body = json::parse(request.body);
                return reply(true, replay_json(build_replay(body.at("sgf"))));
            } catch (const std::exception &error) {
                return reply(false, {}, error.what(), 400);
            }
        });
}
} // namespace weiqi::api
