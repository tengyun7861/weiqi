#include "weiqi/api/http_common.hpp"
#include "weiqi/api/routes.hpp"
#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
void register_system_routes(crow::SimpleApp &app) {
    CROW_ROUTE(app, "/api/v1/health")(
        [] { return reply(true, {{"status", "ok"}, {"version", "0.2.0"}, {"backend", "C++20"}}); });
}
} // namespace weiqi::api
