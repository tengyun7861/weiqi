#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::user_name;
std::optional<GameSnapshot> GameService::join_matchmaking(int user_id, int board_size) {
    if (board_size != 9 && board_size != 13 && board_size != 19)
        throw std::invalid_argument("棋盘大小无效");
    int opponent{};
    {
        std::lock_guard lock(database_.mutex());
        sqlite3_stmt *statement{};
        sqlite3_prepare_v2(database_.handle(),
                           "SELECT user_id FROM matchmaking_queue WHERE board_size=? AND user_id<>? ORDER BY "
                           "joined_at LIMIT 1",
                           -1, &statement, nullptr);
        sqlite3_bind_int(statement, 1, board_size);
        sqlite3_bind_int(statement, 2, user_id);
        if (sqlite3_step(statement) == SQLITE_ROW)
            opponent = sqlite3_column_int(statement, 0);
        sqlite3_finalize(statement);
        if (opponent == 0) {
            sqlite3_prepare_v2(
                database_.handle(),
                "INSERT INTO matchmaking_queue(user_id,board_size) VALUES(?,?) ON CONFLICT(user_id) DO "
                "UPDATE SET board_size=excluded.board_size,joined_at=CURRENT_TIMESTAMP",
                -1, &statement, nullptr);
            sqlite3_bind_int(statement, 1, user_id);
            sqlite3_bind_int(statement, 2, board_size);
            sqlite3_step(statement);
            sqlite3_finalize(statement);
            return std::nullopt;
        }
        sqlite3_prepare_v2(database_.handle(), "DELETE FROM matchmaking_queue WHERE user_id IN (?,?)", -1,
                           &statement, nullptr);
        sqlite3_bind_int(statement, 1, user_id);
        sqlite3_bind_int(statement, 2, opponent);
        sqlite3_step(statement);
        sqlite3_finalize(statement);
    }
    const auto code = create_room(opponent, board_size, false);
    const auto state = join_room(user_id, code, false);
    emit({"match.found", code, opponent, state});
    emit({"match.found", code, user_id, state});
    return state;
}

void GameService::cancel_matchmaking(int user_id) {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt *statement{};
    sqlite3_prepare_v2(database_.handle(), "DELETE FROM matchmaking_queue WHERE user_id=?", -1, &statement,
                       nullptr);
    sqlite3_bind_int(statement, 1, user_id);
    sqlite3_step(statement);
    sqlite3_finalize(statement);
}

} // namespace weiqi
