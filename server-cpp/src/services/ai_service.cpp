#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/detail/scoring.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::score_result;
using detail::user_name;
std::string GameService::create_ai_room(int owner_id, int board_size, int main_time_seconds,
                                        int byo_yomi_seconds) {
    const auto code = create_room(owner_id, board_size, false, main_time_seconds, byo_yomi_seconds);
    std::lock_guard lock(mutex_);
    auto &game = require_game(code);
    game.ai_white = true;
    game.white_name = "KataGo";
    {
        std::lock_guard database_lock(database_.mutex());
        sqlite3_stmt *statement{};
        sqlite3_prepare_v2(database_.handle(), "UPDATE games SET ai_side='white' WHERE id=?", -1, &statement,
                           nullptr);
        sqlite3_bind_int(statement, 1, game.game_id);
        sqlite3_step(statement);
        sqlite3_finalize(statement);
    }
    start_next_turn(game);
    persist_clock(game);
    return code;
}

bool GameService::ai_turn(const std::string &room_code) {
    std::lock_guard lock(mutex_);
    const auto &game = require_game(room_code);
    return game.ai_white && !game.finished && game.board.current_player() == Stone::white;
}
GameSnapshot GameService::ai_resign(const std::string &room_code) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    if (!game.ai_white || game.finished || game.board.current_player() != Stone::white)
        throw std::invalid_argument("当前不是 KataGo 回合");
    if (!settle_clock(game)) {
        finish_game(room_code, game, "B+T", "timeout");
        const auto state = snapshot(room_code, game);
        emit({"game.finished", room_code, std::nullopt, state});
        return state;
    }
    finish_game(room_code, game, "B+R", "resign");
    const auto state = snapshot(room_code, game);
    emit({"game.finished", room_code, std::nullopt, state});
    return state;
}
void GameService::ai_error(const std::string &room_code, const std::string &message) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    if (!game.finished)
        emit({"ai.error", room_code, std::nullopt, snapshot(room_code, game), message});
}
GameSnapshot GameService::ai_move(const std::string &room_code, int x, int y) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    if (!game.ai_white || game.finished || game.board.current_player() != Stone::white)
        throw std::invalid_argument("当前不是 KataGo 回合");
    if (!settle_clock(game)) {
        finish_game(room_code, game, "B+T", "timeout");
        const auto state = snapshot(room_code, game);
        emit({"game.finished", room_code, std::nullopt, state});
        return state;
    }
    const auto result = game.board.play(x, y);
    if (!result.accepted)
        throw std::invalid_argument("KataGo 返回了非法着法：" + result.error);
    game.moves.push_back({Stone::white, x, y});
    persist_move(game, game.moves.back(), result.captured);
    game.passes = 0;
    ++game.sequence;
    start_next_turn(game);
    persist_clock(game);
    const auto state = snapshot(room_code, game);
    emit({"game.updated", room_code, std::nullopt, state});
    return state;
}

GameSnapshot GameService::ai_pass(const std::string &room_code) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    if (!game.ai_white || game.finished || game.board.current_player() != Stone::white)
        throw std::invalid_argument("当前不是 KataGo 回合");
    if (!settle_clock(game)) {
        finish_game(room_code, game, "B+T", "timeout");
        const auto state = snapshot(room_code, game);
        emit({"game.finished", room_code, std::nullopt, state});
        return state;
    }
    game.board.pass();
    game.moves.push_back({Stone::white, -1, -1});
    persist_move(game, game.moves.back(), 0);
    ++game.passes;
    ++game.sequence;
    start_next_turn(game);
    persist_clock(game);
    if (game.passes >= 2) {
        const auto score = game.board.chinese_area_score();
        finish_game(room_code, game, score_result(score), "two_passes");
    }
    const auto state = snapshot(room_code, game);
    emit({game.finished ? "game.finished" : "game.updated", room_code, std::nullopt, state});
    return state;
}

} // namespace weiqi
