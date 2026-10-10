#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/detail/scoring.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::score_result;
using detail::user_name;
void GameService::finish_game(const std::string &room_code, ActiveGame &game, const std::string &result,
                              const std::string &reason) {
    if (game.finished)
        return;
    game.finished = true;
    game.result = result;
    game.finish_reason = reason;
    ++game.sequence;
    const auto sgf = make_sgf(game.board.size(), 7.5, game.moves, result);
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt *statement{};
    sqlite3_prepare_v2(
        database_.handle(),
        "UPDATE games SET result=?,finish_reason=?,ended_at=CURRENT_TIMESTAMP,sgf=? WHERE id=?", -1,
        &statement, nullptr);
    bind_text(statement, 1, result);
    bind_text(statement, 2, reason);
    bind_text(statement, 3, sgf);
    sqlite3_bind_int(statement, 4, game.game_id);
    sqlite3_step(statement);
    sqlite3_finalize(statement);
    sqlite3_prepare_v2(database_.handle(), "UPDATE rooms SET status='finished' WHERE room_code=?", -1,
                       &statement, nullptr);
    bind_text(statement, 1, room_code);
    sqlite3_step(statement);
    sqlite3_finalize(statement);
}

GameSnapshot GameService::move(int user_id, const std::string &room_code, int x, int y) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    if (game.finished)
        throw std::invalid_argument("对局已经结束");
    const int expected = game.board.current_player() == Stone::black ? game.black_user : game.white_user;
    if (expected == 0 || expected != user_id)
        throw std::invalid_argument("当前无权落子");
    if (game.white_user == 0 && !game.ai_white)
        throw std::invalid_argument("请等待另一位棋手加入");
    if (!settle_clock(game)) {
        finish_game(room_code, game, game.board.current_player() == Stone::black ? "W+T" : "B+T", "timeout");
        emit({"game.finished", room_code, std::nullopt, snapshot(room_code, game)});
        throw std::invalid_argument("落子前已超时");
    }
    const auto color = game.board.current_player();
    const auto result = game.board.play(x, y);
    if (!result.accepted)
        throw std::invalid_argument(result.error);
    game.moves.push_back({color, x, y});
    persist_move(game, game.moves.back(), result.captured);
    game.passes = 0;
    ++game.sequence;
    start_next_turn(game);
    persist_clock(game);
    const auto state = snapshot(room_code, game);
    emit({"game.updated", room_code, std::nullopt, state});
    return state;
}

GameSnapshot GameService::pass(int user_id, const std::string &room_code) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    const int expected = game.board.current_player() == Stone::black ? game.black_user : game.white_user;
    if (game.finished || expected == 0 || expected != user_id)
        throw std::invalid_argument("当前无权停一手");
    if (game.white_user == 0 && !game.ai_white)
        throw std::invalid_argument("请等待另一位棋手加入");
    if (!settle_clock(game)) {
        finish_game(room_code, game, game.board.current_player() == Stone::black ? "W+T" : "B+T", "timeout");
        emit({"game.finished", room_code, std::nullopt, snapshot(room_code, game)});
        throw std::invalid_argument("停一手前已超时");
    }
    const auto color = game.board.current_player();
    game.board.pass();
    game.moves.push_back({color, -1, -1});
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

GameSnapshot GameService::resign(int user_id, const std::string &room_code) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    if (user_id != game.black_user && user_id != game.white_user)
        throw std::invalid_argument("观战者不能认输");
    finish_game(room_code, game, user_id == game.black_user ? "W+R" : "B+R", "resign");
    const auto state = snapshot(room_code, game);
    emit({"game.finished", room_code, std::nullopt, state});
    return state;
}

} // namespace weiqi
