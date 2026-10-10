#pragma once
#include "weiqi/auth.hpp"
#include "weiqi/game_service.hpp"
#include "weiqi/katago_gtp.hpp"
#include <crow.h>

namespace weiqi::api {
void register_system_routes(crow::SimpleApp &app);
void register_asset_routes(crow::SimpleApp &app, const std::string &web_root);
void register_auth_routes(crow::SimpleApp &app, AuthService &auth);
void register_room_routes(crow::SimpleApp &app, AuthService &auth, GameService &games);
void register_record_routes(crow::SimpleApp &app, AuthService &auth, GameService &games);
void register_replay_routes(crow::SimpleApp &app, AuthService &auth);
void register_ai_routes(crow::SimpleApp &app, AuthService &auth, GameService &games, KataGoGtp &katago);
} // namespace weiqi::api
