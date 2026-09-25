#include "weiqi/database.hpp"

#include <stdexcept>

namespace weiqi {

Database::Database(std::string file_path) {
    if (sqlite3_open_v2(file_path.c_str(), &db_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
        const std::string error = db_ ? sqlite3_errmsg(db_) : "无法打开 SQLite 数据库";
        if (db_) sqlite3_close(db_);
        throw std::runtime_error(error);
    }
    execute("PRAGMA foreign_keys = ON;");
    execute("PRAGMA journal_mode = WAL;");
}

Database::~Database() { if (db_) sqlite3_close(db_); }

void Database::execute(const char* sql) {
    char* error{};
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error ? error : "SQLite 执行失败";
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

void Database::add_column_if_missing(const char* table, const char* column, const char* definition) {
    sqlite3_stmt* statement{}; sqlite3_prepare_v2(db_, (std::string("PRAGMA table_info(") + table + ')').c_str(), -1, &statement, nullptr);
    bool found{}; while (sqlite3_step(statement) == SQLITE_ROW) { const auto name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1)); if (name && std::string(name) == column) { found = true; break; } } sqlite3_finalize(statement);
    if (!found) execute((std::string("ALTER TABLE ") + table + " ADD COLUMN " + definition).c_str());
}

void Database::migrate() {
    std::lock_guard lock(mutex_);
    execute(R"SQL(
CREATE TABLE IF NOT EXISTS schema_migrations(version INTEGER PRIMARY KEY);
CREATE TABLE IF NOT EXISTS users(
 id INTEGER PRIMARY KEY, nickname TEXT NOT NULL UNIQUE COLLATE NOCASE,
 password_hash TEXT NOT NULL, created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 rating INTEGER NOT NULL DEFAULT 1500);
CREATE TABLE IF NOT EXISTS sessions(
 token_hash TEXT PRIMARY KEY, user_id INTEGER NOT NULL REFERENCES users(id), expires_at INTEGER NOT NULL);
CREATE TABLE IF NOT EXISTS rooms(
 id INTEGER PRIMARY KEY, room_code TEXT NOT NULL UNIQUE, board_size INTEGER NOT NULL,
 is_public INTEGER NOT NULL, rules TEXT NOT NULL DEFAULT 'chinese',
 main_time_seconds INTEGER NOT NULL, byo_yomi_seconds INTEGER NOT NULL,
 owner_id INTEGER NOT NULL REFERENCES users(id), status TEXT NOT NULL DEFAULT 'waiting', created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);
CREATE TABLE IF NOT EXISTS games(
 id INTEGER PRIMARY KEY, room_id INTEGER REFERENCES rooms(id), black_user_id INTEGER REFERENCES users(id),
 white_user_id INTEGER REFERENCES users(id), result TEXT, finish_reason TEXT, ai_side TEXT, komi REAL NOT NULL DEFAULT 7.5,
 started_at TEXT, ended_at TEXT, sgf TEXT,
 black_time_seconds INTEGER NOT NULL DEFAULT 600, white_time_seconds INTEGER NOT NULL DEFAULT 600,
 consecutive_passes INTEGER NOT NULL DEFAULT 0, turn_started_epoch INTEGER NOT NULL DEFAULT 0,
 black_byo_remaining INTEGER NOT NULL DEFAULT 30, white_byo_remaining INTEGER NOT NULL DEFAULT 30);
CREATE TABLE IF NOT EXISTS moves(
 id INTEGER PRIMARY KEY, game_id INTEGER NOT NULL REFERENCES games(id), move_number INTEGER NOT NULL,
 color TEXT NOT NULL, x INTEGER, y INTEGER, captured INTEGER NOT NULL DEFAULT 0, created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
 UNIQUE(game_id, move_number));
CREATE TABLE IF NOT EXISTS chat_messages(
 id INTEGER PRIMARY KEY, room_id INTEGER NOT NULL REFERENCES rooms(id), user_id INTEGER NOT NULL REFERENCES users(id),
 content TEXT NOT NULL, created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);
CREATE TABLE IF NOT EXISTS matchmaking_queue(
 user_id INTEGER PRIMARY KEY REFERENCES users(id), board_size INTEGER NOT NULL,
 joined_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);
CREATE INDEX IF NOT EXISTS idx_rooms_public_status ON rooms(is_public, status);
CREATE INDEX IF NOT EXISTS idx_games_player ON games(black_user_id, white_user_id);
CREATE INDEX IF NOT EXISTS idx_moves_game ON moves(game_id, move_number);
INSERT OR IGNORE INTO schema_migrations(version) VALUES(1);
)SQL");
    add_column_if_missing("games", "black_time_seconds", "black_time_seconds INTEGER NOT NULL DEFAULT 600");
    add_column_if_missing("games", "white_time_seconds", "white_time_seconds INTEGER NOT NULL DEFAULT 600");
    add_column_if_missing("games", "consecutive_passes", "consecutive_passes INTEGER NOT NULL DEFAULT 0");
    add_column_if_missing("games", "turn_started_epoch", "turn_started_epoch INTEGER NOT NULL DEFAULT 0");
    add_column_if_missing("games", "ai_side", "ai_side TEXT");
    add_column_if_missing("games", "black_byo_remaining", "black_byo_remaining INTEGER NOT NULL DEFAULT 30");
    add_column_if_missing("games", "white_byo_remaining", "white_byo_remaining INTEGER NOT NULL DEFAULT 30");
}

} // namespace weiqi
