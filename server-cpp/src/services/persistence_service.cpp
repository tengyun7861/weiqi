#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::user_name;
void GameService::persist_move(const ActiveGame &game, const SgfMove &move, int captured) {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt *statement{};
    sqlite3_prepare_v2(database_.handle(),
                       "INSERT INTO moves(game_id,move_number,color,x,y,captured) VALUES(?,?,?,?,?,?)", -1,
                       &statement, nullptr);
    sqlite3_bind_int(statement, 1, game.game_id);
    sqlite3_bind_int(statement, 2, static_cast<int>(game.moves.size()));
    sqlite3_bind_text(statement, 3, move.color == Stone::black ? "black" : "white", -1, SQLITE_STATIC);
    if (move.x < 0)
        sqlite3_bind_null(statement, 4);
    else
        sqlite3_bind_int(statement, 4, move.x);
    if (move.y < 0)
        sqlite3_bind_null(statement, 5);
    else
        sqlite3_bind_int(statement, 5, move.y);
    sqlite3_bind_int(statement, 6, captured);
    const int result = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE)
        throw std::runtime_error("无法保存着法");
}

void GameService::persist_clock(const ActiveGame &game) {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt *statement{};
    sqlite3_prepare_v2(database_.handle(),
                       "UPDATE games SET "
                       "black_time_seconds=?,white_time_seconds=?,black_byo_remaining=?,white_byo_remaining=?"
                       ",consecutive_passes=?,turn_started_epoch=strftime('%s','now') WHERE id=?",
                       -1, &statement, nullptr);
    sqlite3_bind_int(statement, 1, game.black_time);
    sqlite3_bind_int(statement, 2, game.white_time);
    sqlite3_bind_int(statement, 3, game.black_byo_remaining);
    sqlite3_bind_int(statement, 4, game.white_byo_remaining);
    sqlite3_bind_int(statement, 5, game.passes);
    sqlite3_bind_int(statement, 6, game.game_id);
    sqlite3_step(statement);
    sqlite3_finalize(statement);
}

} // namespace weiqi
