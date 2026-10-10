#include "weiqi/api/routes.hpp"
#include "weiqi/web_assets.hpp"
#include <memory>

namespace weiqi::api {
void register_asset_routes(crow::SimpleApp &app, const std::string &web_root) {
    auto assets = std::make_shared<WebAssets>(web_root);
    auto serve = [assets](const std::string &path) {
        const auto asset = assets->resolve(path);
        if (!asset)
            return crow::response(404, "页面或资源不存在。请先构建 client-web。");
        crow::response response;
        response.set_static_file_info_unsafe(asset->file.string());
        response.set_header("Content-Type", asset->content_type);
        response.set_header("Cache-Control",
                            asset->immutable ? "public, max-age=31536000, immutable" : "no-cache");
        response.set_header("X-Content-Type-Options", "nosniff");
        response.set_header(
            "Content-Security-Policy",
            "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; "
            "connect-src 'self'; frame-ancestors 'none'; base-uri 'self'");
        return response;
    };
    CROW_ROUTE(app, "/")([serve] { return serve("index.html"); });
    CROW_ROUTE(app, "/assets/<path>")([serve](const std::string &path) { return serve("assets/" + path); });
    CROW_ROUTE(app, "/favicon.svg")([serve] { return serve("favicon.svg"); });
}
} // namespace weiqi::api
