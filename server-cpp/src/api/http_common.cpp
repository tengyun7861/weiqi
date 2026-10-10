#include "weiqi/api/http_common.hpp"

namespace weiqi::api {
crow::response reply(bool ok, json data, std::string error, int status) {
    crow::response response(
        status,
        json{{"ok", ok}, {"data", std::move(data)}, {"error", error.empty() ? json(nullptr) : json(error)}}
            .dump());
    response.set_header("Content-Type", "application/json; charset=utf-8");
    response.set_header("Cache-Control", "no-store");
    response.set_header("X-Content-Type-Options", "nosniff");
    return response;
}
std::string bearer(const crow::request &request) {
    const auto header = request.get_header_value("Authorization");
    return header.starts_with("Bearer ") ? header.substr(7) : std::string{};
}
std::optional<AuthenticatedUser> user_for(const crow::request &request, AuthService &auth) {
    return auth.authenticate(bearer(request));
}
crow::response unauthorized() {
    return reply(false, {}, "登录已过期，请重新登录", 401);
}
} // namespace weiqi::api
