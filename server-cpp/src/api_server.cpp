#include "weiqi/api_server.hpp"
#include "weiqi/api/realtime_server.hpp"
#include "weiqi/api/routes.hpp"
#include <iostream>
#include <thread>

namespace weiqi {
int run_api_server(AuthService &auth, GameService &games, KataGoGtp &katago, unsigned short port,
                   const std::string &web_root) {
    crow::SimpleApp app;
    app.loglevel(crow::LogLevel::Warning);
    api::register_system_routes(app);
    api::register_asset_routes(app, web_root);
    api::register_auth_routes(app, auth);
    api::register_room_routes(app, auth, games);
    api::register_record_routes(app, auth, games);
    api::register_replay_routes(app, auth);
    api::register_ai_routes(app, auth, games, katago);
    api::RealtimeServer realtime(auth, games, katago);
    realtime.attach(app);
    std::jthread clock([&](std::stop_token stop) {
        while (!stop.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (stop.stop_requested())
                break;
            try {
                games.tick();
            } catch (const std::exception &error) {
                std::cerr << "计时更新失败：" << error.what() << '\n';
            }
        }
    });
    app.port(port).multithreaded().run();
    return 0;
}
} // namespace weiqi
