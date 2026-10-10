#include "weiqi/api/http_common.hpp"
#include "weiqi/api/routes.hpp"
#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
void register_auth_routes(crow::SimpleApp &app, AuthService &auth) {
    CROW_ROUTE(app, "/api/v1/auth/register")
        .methods(crow::HTTPMethod::Post)([&](const crow::request &request) {
            try {
                if (request.body.size() > 4096)
                    return reply(false, {}, "注册信息过长", 413);
                const auto body = json::parse(request.body);
                auth.register_user(body.at("nickname"), body.at("password"));
                return reply(true, {{"message", "注册成功，请登录"}}, {}, 201);
            } catch (const std::exception &error) {
                return reply(false, {}, error.what(), 400);
            }
        });
    CROW_ROUTE(app, "/api/v1/auth/login").methods(crow::HTTPMethod::Post)([&](const crow::request &request) {
        try {
            if (request.body.size() > 4096)
                return reply(false, {}, "登录信息过长", 413);
            const auto body = json::parse(request.body);
            const auto token = auth.login(body.at("nickname"), body.at("password"));
            const auto user = auth.authenticate(token);
            return reply(true, {{"token", token},
                                {"user", {{"id", user->id}, {"nickname", user->nickname}}},
                                {"expiresIn", 86400}});
        } catch (const std::exception &error) {
            return reply(false, {}, error.what(), 401);
        }
    });
    CROW_ROUTE(app, "/api/v1/auth/me")([&](const crow::request &request) {
        const auto user = user_for(request, auth);
        return user ? reply(true, {{"id", user->id}, {"nickname", user->nickname}}) : unauthorized();
    });
    CROW_ROUTE(app, "/api/v1/auth/logout").methods(crow::HTTPMethod::Post)([&](const crow::request &request) {
        try {
            auth.revoke(bearer(request));
            return reply(true, {{"message", "已退出登录"}});
        } catch (const std::exception &error) {
            return reply(false, {}, error.what(), 500);
        }
    });
}
} // namespace weiqi::api
