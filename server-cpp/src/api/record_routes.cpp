#include "weiqi/api/http_common.hpp"
#include "weiqi/api/routes.hpp"
#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
void register_record_routes(crow::SimpleApp &app, AuthService &auth, GameService &games) {
    CROW_ROUTE(app, "/api/v1/games").methods(crow::HTTPMethod::Get)([&](const crow::request &request) {
        const auto user = user_for(request, auth);
        if (!user)
            return unauthorized();
        json records = json::array();
        for (const auto &record : games.game_records(user->id))
            if (!record.ended_at.empty())
                records.push_back({{"id", record.id},
                                   {"result", record.result},
                                   {"finishReason", record.finish_reason},
                                   {"endedAt", record.ended_at}});
        return reply(true, std::move(records));
    });
    CROW_ROUTE(app, "/api/v1/games/<int>/sgf")([&](const crow::request &request, int id) {
        const auto user = user_for(request, auth);
        if (!user)
            return unauthorized();
        try {
            return reply(true, {{"sgf", games.game_sgf(user->id, id)}});
        } catch (const std::exception &error) {
            return reply(false, {}, error.what(), 404);
        }
    });
    CROW_ROUTE(app, "/api/v1/games/<int>/replay")([&](const crow::request &request, int id) {
        const auto user = user_for(request, auth);
        if (!user)
            return unauthorized();
        try {
            return reply(true, replay_json(build_replay(games.game_sgf(user->id, id))));
        } catch (const std::exception &error) {
            return reply(false, {}, error.what(), 400);
        }
    });
}
} // namespace weiqi::api
