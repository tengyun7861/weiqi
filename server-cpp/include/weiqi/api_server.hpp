#pragma once

#include "weiqi/auth.hpp"
#include "weiqi/game_service.hpp"
#include "weiqi/katago_gtp.hpp"

namespace weiqi {

int run_api_server(AuthService& auth, GameService& games, KataGoGtp& katago, unsigned short port);

} // namespace weiqi
