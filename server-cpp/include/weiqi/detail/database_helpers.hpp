#pragma once
#include "weiqi/database.hpp"
#include <stdexcept>
#include <string>

namespace weiqi::detail {
inline void bind_text(sqlite3_stmt *statement, int index, const std::string &value) {
    sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT);
}
inline std::string user_name(sqlite3 *db, int user) {
    sqlite3_stmt *statement{};
    if (sqlite3_prepare_v2(db, "SELECT nickname FROM users WHERE id=?", -1, &statement, nullptr) != SQLITE_OK)
        throw std::runtime_error("无法读取棋手信息");
    sqlite3_bind_int(statement, 1, user);
    std::string name;
    if (sqlite3_step(statement) == SQLITE_ROW)
        name = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
    sqlite3_finalize(statement);
    return name;
}
} // namespace weiqi::detail
