#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <random>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::user_name;
namespace {
std::string new_room_code() {
    constexpr char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<int> distribution(0, static_cast<int>(sizeof(alphabet) - 2));
    std::string code(6, 'A');
    for (auto &character : code)
        character = alphabet[distribution(generator)];
    return code;
}
} // namespace

std::string GameService::create_room(int owner_id, int board_size, bool is_public, int main_time_seconds,
                                     int byo_yomi_seconds) {
    if (board_size != 9 && board_size != 13 && board_size != 19)
        throw std::invalid_argument("棋盘大小只能是 9、13 或 19 路");
    if (main_time_seconds < 0 || byo_yomi_seconds < 1)
        throw std::invalid_argument("计时配置无效");
    std::string code;
    std::string owner_name;
    int game_id{};
    {
        std::lock_guard lock(database_.mutex());
        sqlite3_stmt *statement{};
        owner_name = user_name(database_.handle(), owner_id);
        for (int attempt = 0; attempt < 10; ++attempt) {
            code = new_room_code();
            sqlite3_prepare_v2(database_.handle(),
                               "INSERT INTO "
                               "rooms(room_code,board_size,is_public,main_time_seconds,byo_yomi_seconds,"
                               "owner_id) VALUES(?,?,?,?,?,?)",
                               -1, &statement, nullptr);
            bind_text(statement, 1, code);
            sqlite3_bind_int(statement, 2, board_size);
            sqlite3_bind_int(statement, 3, is_public);
            sqlite3_bind_int(statement, 4, main_time_seconds);
            sqlite3_bind_int(statement, 5, byo_yomi_seconds);
            sqlite3_bind_int(statement, 6, owner_id);
            const int result = sqlite3_step(statement);
            sqlite3_finalize(statement);
            if (result == SQLITE_DONE)
                break;
            code.clear();
        }
        if (code.empty())
            throw std::runtime_error("无法生成房间码");
        sqlite3_prepare_v2(
            database_.handle(),
            "INSERT INTO "
            "games(room_id,black_user_id,komi,started_at,black_time_seconds,white_time_seconds,turn_started_"
            "epoch) SELECT id,?,7.5,CURRENT_TIMESTAMP,?,?,strftime('%s','now') FROM rooms WHERE room_code=?",
            -1, &statement, nullptr);
        sqlite3_bind_int(statement, 1, owner_id);
        sqlite3_bind_int(statement, 2, main_time_seconds);
        sqlite3_bind_int(statement, 3, main_time_seconds);
        bind_text(statement, 4, code);
        if (sqlite3_step(statement) != SQLITE_DONE) {
            sqlite3_finalize(statement);
            throw std::runtime_error("无法创建对局");
        }
        sqlite3_finalize(statement);
        game_id = static_cast<int>(sqlite3_last_insert_rowid(database_.handle()));
    }
    std::lock_guard lock(mutex_);
    auto [it, inserted] = active_games_.emplace(std::piecewise_construct, std::forward_as_tuple(code),
                                                std::forward_as_tuple(board_size));
    auto &game = it->second;
    game.black_user = owner_id;
    game.black_name = owner_name;
    game.black_time = main_time_seconds;
    game.white_time = main_time_seconds;
    game.black_byo_remaining = byo_yomi_seconds;
    game.white_byo_remaining = byo_yomi_seconds;
    game.byo_yomi_seconds = byo_yomi_seconds;
    game.game_id = game_id;
    return code;
}

std::vector<RoomSummary> GameService::public_rooms() const {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt *statement{};
    sqlite3_prepare_v2(database_.handle(),
                       "SELECT room_code,board_size,is_public,status,owner_id FROM rooms WHERE is_public=1 "
                       "AND status IN ('waiting','playing') ORDER BY id DESC",
                       -1, &statement, nullptr);
    std::vector<RoomSummary> rooms;
    while (sqlite3_step(statement) == SQLITE_ROW)
        rooms.push_back({reinterpret_cast<const char *>(sqlite3_column_text(statement, 0)),
                         sqlite3_column_int(statement, 1), sqlite3_column_int(statement, 2) != 0,
                         reinterpret_cast<const char *>(sqlite3_column_text(statement, 3)),
                         sqlite3_column_int(statement, 4)});
    sqlite3_finalize(statement);
    return rooms;
}

GameSnapshot GameService::join_room(int user_id, const std::string &room_code, bool spectate) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    if (game.finished && !spectate && user_id != game.black_user && user_id != game.white_user)
        throw std::invalid_argument("对局已结束，可以以观战身份进入");
    if (!spectate && game.ai_white && user_id != game.black_user)
        throw std::invalid_argument("这是人机对局，只能以观战身份进入");
    if (!spectate && game.white_user == 0 && !game.ai_white && user_id != game.black_user) {
        game.white_user = user_id;
        std::lock_guard database_lock(database_.mutex());
        sqlite3_stmt *statement{};
        game.white_name = user_name(database_.handle(), user_id);
        start_next_turn(game);
        sqlite3_prepare_v2(
            database_.handle(),
            "UPDATE games SET white_user_id=?,turn_started_epoch=strftime('%s','now') WHERE id=?", -1,
            &statement, nullptr);
        sqlite3_bind_int(statement, 1, user_id);
        sqlite3_bind_int(statement, 2, game.game_id);
        sqlite3_step(statement);
        sqlite3_finalize(statement);
        sqlite3_prepare_v2(database_.handle(), "UPDATE rooms SET status='playing' WHERE room_code=?", -1,
                           &statement, nullptr);
        bind_text(statement, 1, room_code);
        sqlite3_step(statement);
        sqlite3_finalize(statement);
    } else if (!spectate && user_id != game.black_user && user_id != game.white_user)
        throw std::invalid_argument("房间已满，请以观战身份进入");
    const auto state = snapshot(room_code, game);
    emit({"room.snapshot", room_code, user_id, state});
    return state;
}

void GameService::leave_room(int user_id, const std::string &room_code) {
    std::lock_guard lock(mutex_);
    auto &game = require_game(room_code);
    if (!game.finished && (game.black_user == user_id || game.white_user == user_id))
        emit({"game.updated", room_code, std::nullopt, snapshot(room_code, game), "对手暂时断线"});
}

} // namespace weiqi
