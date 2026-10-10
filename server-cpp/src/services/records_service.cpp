#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::user_name;
std::vector<GameRecordSummary> GameService::game_records(int user_id) const {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt *statement{};
    sqlite3_prepare_v2(database_.handle(),
                       "SELECT id,COALESCE(result,''),COALESCE(finish_reason,''),COALESCE(ended_at,'') FROM "
                       "games WHERE black_user_id=? OR white_user_id=? ORDER BY id DESC",
                       -1, &statement, nullptr);
    sqlite3_bind_int(statement, 1, user_id);
    sqlite3_bind_int(statement, 2, user_id);
    std::vector<GameRecordSummary> records;
    while (sqlite3_step(statement) == SQLITE_ROW)
        records.push_back({sqlite3_column_int(statement, 0),
                           reinterpret_cast<const char *>(sqlite3_column_text(statement, 1)),
                           reinterpret_cast<const char *>(sqlite3_column_text(statement, 2)),
                           reinterpret_cast<const char *>(sqlite3_column_text(statement, 3))});
    sqlite3_finalize(statement);
    return records;
}

std::string GameService::game_sgf(int user_id, int game_id) const {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt *statement{};
    sqlite3_prepare_v2(
        database_.handle(),
        "SELECT COALESCE(sgf,'') FROM games WHERE id=? AND (black_user_id=? OR white_user_id=?)", -1,
        &statement, nullptr);
    sqlite3_bind_int(statement, 1, game_id);
    sqlite3_bind_int(statement, 2, user_id);
    sqlite3_bind_int(statement, 3, user_id);
    if (sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        throw std::invalid_argument("棋谱不存在或无权访问");
    }
    const auto text = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
    std::string sgf = text ? text : "";
    sqlite3_finalize(statement);
    return sgf;
}

} // namespace weiqi
