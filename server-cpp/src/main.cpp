#include "weiqi/api_server.hpp"
#include "weiqi/auth.hpp"
#include "weiqi/database.hpp"
#include "weiqi/game_service.hpp"
#include "weiqi/katago_gtp.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char *argv[]) {
    try {
        const std::string database_path = argc > 1 ? argv[1] : "weiqi.sqlite3";
        const int configured_port = argc > 2 ? std::stoi(argv[2]) : 8081;
        if (configured_port < 1 || configured_port > 65535)
            throw std::invalid_argument("端口须为 1–65535");
        const auto port = static_cast<unsigned short>(configured_port);
        const auto parent = std::filesystem::path(database_path).parent_path();
        if (!parent.empty())
            std::filesystem::create_directories(parent);
        std::string web_root = argc > 3 ? argv[3] : WEIQI_DEFAULT_WEB_ROOT;
        const auto packaged_web = std::filesystem::absolute(argv[0]).parent_path() / "web";
        if (argc <= 3 && std::filesystem::is_regular_file(packaged_web / "index.html"))
            web_root = packaged_web.string();
        weiqi::Database database(database_path);
        database.migrate();
        weiqi::AuthService auth(database);
        weiqi::GameService games(database);
        weiqi::KataGoGtp katago;
        std::cout << "弈境围棋网站：http://127.0.0.1:" << port << "\n网页目录：" << web_root << '\n';
        return weiqi::run_api_server(auth, games, katago, port, web_root);
    } catch (const std::exception &error) {
        std::cerr << "服务端启动失败：" << error.what() << '\n';
        return 1;
    }
}
