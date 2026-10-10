#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::user_name;
void GameService::start_next_turn(ActiveGame &game) {
    if (game.board.current_player() == Stone::black && game.black_time <= 0) {
        game.black_byo_remaining = game.byo_yomi_seconds;
        game.black_fraction = std::chrono::milliseconds::zero();
    }
    if (game.board.current_player() == Stone::white && game.white_time <= 0) {
        game.white_byo_remaining = game.byo_yomi_seconds;
        game.white_fraction = std::chrono::milliseconds::zero();
    }
    game.turn_started = std::chrono::steady_clock::now();
}

bool GameService::charge_elapsed(ActiveGame &game, int elapsed_seconds) {
    int &main = game.board.current_player() == Stone::black ? game.black_time : game.white_time;
    int &byo =
        game.board.current_player() == Stone::black ? game.black_byo_remaining : game.white_byo_remaining;
    if (elapsed_seconds <= 0)
        return true;
    if (main > 0) {
        const int used = std::min(main, elapsed_seconds);
        main -= used;
        elapsed_seconds -= used;
    }
    if (elapsed_seconds > 0)
        byo -= elapsed_seconds;
    return main > 0 || byo > 0;
}

bool GameService::settle_clock(ActiveGame &game) {
    const auto now = std::chrono::steady_clock::now();
    if (game.finished || (game.white_user == 0 && !game.ai_white)) {
        game.turn_started = now;
        return true;
    }
    auto &fraction = game.board.current_player() == Stone::black ? game.black_fraction : game.white_fraction;
    const auto elapsed =
        fraction + std::chrono::duration_cast<std::chrono::milliseconds>(now - game.turn_started);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed);
    fraction = elapsed - seconds;
    game.turn_started = now;
    return charge_elapsed(game, static_cast<int>(seconds.count()));
}

void GameService::tick() {
    std::lock_guard lock(mutex_);
    for (auto &[code, game] : active_games_) {
        if (game.finished || (game.white_user == 0 && !game.ai_white))
            continue;
        const bool alive = settle_clock(game);
        ++game.sequence;
        persist_clock(game);
        if (!alive) {
            finish_game(code, game, game.board.current_player() == Stone::black ? "W+T" : "B+T", "timeout");
            emit({"game.finished", code, std::nullopt, snapshot(code, game)});
        } else
            emit({"clock.updated", code, std::nullopt, snapshot(code, game)});
    }
}
} // namespace weiqi
