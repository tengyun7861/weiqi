#include "weiqi/api_server.hpp"
#include "weiqi/auth.hpp"
#include "weiqi/database.hpp"
#include "weiqi/game_service.hpp"
#include "weiqi/katago_gtp.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        const std::string database_path = argc > 1 ? argv[1] : "weiqi.sqlite3";
        const auto port = static_cast<unsigned short>(argc > 2 ? std::stoi(argv[2]) : 8081);
        weiqi::Database database(database_path); database.migrate();
        weiqi::AuthService auth(database); weiqi::GameService games(database); weiqi::KataGoGtp katago;
        std::cout << "弈境围棋服务端正在监听 0.0.0.0:" << port << '\n';
        return weiqi::run_api_server(auth, games, katago, port);
    } catch (const std::exception& error) {
        std::cerr << "服务端启动失败：" << error.what() << '\n'; return 1;
    }
}
