#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::user_name;
GameService::ActiveGame &GameService::require_game(const std::string &room_code) {
    const auto it = active_games_.find(room_code);
    return it == active_games_.end() ? load_game(room_code) : it->second;
}

GameService::ActiveGame &GameService::load_game(const std::string &room_code) {
    int game_id{}, size{}, black{}, white{}, black_clock{}, white_clock{}, black_byo{}, white_byo{}, passes{},
        byo{};
    long long turn_epoch{};
    bool finished{}, ai_white{};
    std::vector<SgfMove> moves;
    {
        std::lock_guard lock(database_.mutex());
        sqlite3_stmt *statement{};
        sqlite3_prepare_v2(
            database_.handle(),
            "SELECT "
            "g.id,r.board_size,g.black_user_id,COALESCE(g.white_user_id,0),g.black_time_seconds,g.white_time_"
            "seconds,g.black_byo_remaining,g.white_byo_remaining,g.consecutive_passes,r.byo_yomi_seconds,g."
            "turn_started_epoch,g.ended_at IS NOT NULL,COALESCE(g.ai_side,'') FROM games g JOIN rooms r ON "
            "r.id=g.room_id WHERE r.room_code=? ORDER BY g.id DESC LIMIT 1",
            -1, &statement, nullptr);
        bind_text(statement, 1, room_code);
        if (sqlite3_step(statement) != SQLITE_ROW) {
            sqlite3_finalize(statement);
            throw std::invalid_argument("房间不存在");
        }
        game_id = sqlite3_column_int(statement, 0);
        size = sqlite3_column_int(statement, 1);
        black = sqlite3_column_int(statement, 2);
        white = sqlite3_column_int(statement, 3);
        black_clock = sqlite3_column_int(statement, 4);
        white_clock = sqlite3_column_int(statement, 5);
        black_byo = sqlite3_column_int(statement, 6);
        white_byo = sqlite3_column_int(statement, 7);
        passes = sqlite3_column_int(statement, 8);
        byo = sqlite3_column_int(statement, 9);
        turn_epoch = sqlite3_column_int64(statement, 10);
        finished = sqlite3_column_int(statement, 11) != 0;
        ai_white = std::string(reinterpret_cast<const char *>(sqlite3_column_text(statement, 12))) == "white";
        sqlite3_finalize(statement);
        sqlite3_prepare_v2(database_.handle(),
                           "SELECT color,x,y FROM moves WHERE game_id=? ORDER BY move_number", -1, &statement,
                           nullptr);
        sqlite3_bind_int(statement, 1, game_id);
        while (sqlite3_step(statement) == SQLITE_ROW) {
            const auto color =
                std::string(reinterpret_cast<const char *>(sqlite3_column_text(statement, 0))) == "black"
                    ? Stone::black
                    : Stone::white;
            const int x =
                sqlite3_column_type(statement, 1) == SQLITE_NULL ? -1 : sqlite3_column_int(statement, 1);
            const int y =
                sqlite3_column_type(statement, 2) == SQLITE_NULL ? -1 : sqlite3_column_int(statement, 2);
            moves.push_back({color, x, y});
        }
        sqlite3_finalize(statement);
    }
    auto [it, inserted] = active_games_.emplace(std::piecewise_construct, std::forward_as_tuple(room_code),
                                                std::forward_as_tuple(size));
    auto &game = it->second;
    game.game_id = game_id;
    game.black_user = black;
    game.white_user = white;
    game.black_time = black_clock;
    game.white_time = white_clock;
    game.black_byo_remaining = black_byo > 0 ? black_byo : byo;
    game.white_byo_remaining = white_byo > 0 ? white_byo : byo;
    game.byo_yomi_seconds = byo;
    game.passes = passes;
    game.finished = finished;
    game.ai_white = ai_white;
    {
        std::lock_guard database_lock(database_.mutex());
        game.black_name = user_name(database_.handle(), black);
        game.white_name = ai_white ? "KataGo" : user_name(database_.handle(), white);
        sqlite3_stmt *statement{};
        sqlite3_prepare_v2(database_.handle(),
                           "SELECT COALESCE(result,''),COALESCE(finish_reason,'') FROM games WHERE id=?", -1,
                           &statement, nullptr);
        sqlite3_bind_int(statement, 1, game_id);
        if (sqlite3_step(statement) == SQLITE_ROW) {
            game.result = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
            game.finish_reason = reinterpret_cast<const char *>(sqlite3_column_text(statement, 1));
        }
        sqlite3_finalize(statement);
    }
    for (const auto &move : moves) {
        if (game.board.current_player() != move.color)
            throw std::runtime_error("数据库棋谱顺序损坏");
        const auto result = move.x < 0 ? MoveResult{true, {}, 0} : game.board.play(move.x, move.y);
        if (move.x < 0)
            game.board.pass();
        if (!result.accepted)
            throw std::runtime_error("数据库棋谱包含非法着法");
        game.moves.push_back(move);
    }
    game.sequence = static_cast<int>(moves.size());
    const auto now_epoch =
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    const bool timed_out = !game.finished && (game.white_user != 0 || game.ai_white) && turn_epoch > 0 &&
                           now_epoch > turn_epoch &&
                           !charge_elapsed(game, static_cast<int>(now_epoch - turn_epoch));
    if (timed_out)
        finish_game(room_code, game, game.board.current_player() == Stone::black ? "W+T" : "B+T", "timeout");
    game.turn_started = std::chrono::steady_clock::now();
    persist_clock(game);
    return game;
}

GameSnapshot GameService::snapshot(const std::string &code, const ActiveGame &game) {
    GameSnapshot result{code,
                        game.board.size(),
                        game.board.current_player(),
                        {},
                        game.black_time,
                        game.white_time,
                        game.black_byo_remaining,
                        game.white_byo_remaining,
                        game.byo_yomi_seconds,
                        game.passes,
                        game.finished,
                        game.sequence,
                        game.black_user,
                        game.white_user,
                        game.ai_white};
    result.black_name = game.black_name;
    result.white_name = game.white_name;
    result.result = game.result;
    result.finish_reason = game.finish_reason;
    result.move_count = static_cast<int>(game.moves.size());
    if (!game.moves.empty()) {
        result.last_x = game.moves.back().x;
        result.last_y = game.moves.back().y;
    }
    result.stones.reserve(game.board.size() * game.board.size());
    for (int y = 0; y < game.board.size(); ++y)
        for (int x = 0; x < game.board.size(); ++x)
            result.stones.push_back(static_cast<int>(game.board.at(x, y)));
    return result;
}

GameSnapshot GameService::room_state(const std::string &room_code) {
    std::lock_guard lock(mutex_);
    return snapshot(room_code, require_game(room_code));
}

std::vector<SgfMove> GameService::moves_for_room(const std::string &room_code) {
    std::lock_guard lock(mutex_);
    return require_game(room_code).moves;
}
void GameService::set_event_listener(std::function<void(const RealtimeEvent &)> listener) {
    std::lock_guard lock(mutex_);
    event_listener_ = std::move(listener);
}
void GameService::emit(RealtimeEvent event) const {
    if (event_listener_)
        event_listener_(event);
}

} // namespace weiqi
