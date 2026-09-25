#include "weiqi/game_service.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <random>
#include <sstream>
#include <stdexcept>

namespace weiqi {
namespace {
std::string new_room_code() {
    constexpr char alphabet[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    std::random_device device; std::mt19937 generator(device());
    std::uniform_int_distribution<int> distribution(0, static_cast<int>(sizeof(alphabet) - 2));
    std::string code(6, 'A'); for (auto& character : code) character = alphabet[distribution(generator)]; return code;
}
void bind_text(sqlite3_stmt* statement, int index, const std::string& value) { sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT); }
std::string score_result(const AreaScore& score) {
    const double margin = static_cast<double>(score.black) - (static_cast<double>(score.white) + 7.5);
    std::ostringstream text; text << std::fixed << std::setprecision(1) << std::abs(margin);
    auto result=text.str(); if(result.ends_with(".0")) result.resize(result.size()-2);
    return std::string(margin > 0 ? "B+" : "W+") + result;
}
}

std::string GameService::create_room(int owner_id, int board_size, bool is_public, int main_time_seconds, int byo_yomi_seconds) {
    if (board_size != 9 && board_size != 13 && board_size != 19) throw std::invalid_argument("棋盘大小只能是 9、13 或 19 路");
    if (main_time_seconds < 0 || byo_yomi_seconds < 1) throw std::invalid_argument("计时配置无效");
    std::string code; int game_id{};
    { std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{};
        for (int attempt = 0; attempt < 10; ++attempt) {
            code = new_room_code(); sqlite3_prepare_v2(database_.handle(), "INSERT INTO rooms(room_code,board_size,is_public,main_time_seconds,byo_yomi_seconds,owner_id) VALUES(?,?,?,?,?,?)", -1, &statement, nullptr);
            bind_text(statement, 1, code); sqlite3_bind_int(statement, 2, board_size); sqlite3_bind_int(statement, 3, is_public); sqlite3_bind_int(statement, 4, main_time_seconds); sqlite3_bind_int(statement, 5, byo_yomi_seconds); sqlite3_bind_int(statement, 6, owner_id);
            const int result = sqlite3_step(statement); sqlite3_finalize(statement); if (result == SQLITE_DONE) break; code.clear();
        }
        if (code.empty()) throw std::runtime_error("无法生成房间码");
        sqlite3_prepare_v2(database_.handle(), "INSERT INTO games(room_id,black_user_id,komi,started_at,black_time_seconds,white_time_seconds,turn_started_epoch) SELECT id,?,7.5,CURRENT_TIMESTAMP,?,?,strftime('%s','now') FROM rooms WHERE room_code=?", -1, &statement, nullptr);
        sqlite3_bind_int(statement, 1, owner_id); sqlite3_bind_int(statement, 2, main_time_seconds); sqlite3_bind_int(statement, 3, main_time_seconds); bind_text(statement, 4, code); if (sqlite3_step(statement) != SQLITE_DONE) { sqlite3_finalize(statement); throw std::runtime_error("无法创建对局"); }
        sqlite3_finalize(statement); game_id = static_cast<int>(sqlite3_last_insert_rowid(database_.handle()));
    }
    std::lock_guard lock(mutex_); auto [it, inserted] = active_games_.emplace(std::piecewise_construct, std::forward_as_tuple(code), std::forward_as_tuple(board_size));
    auto& game = it->second; game.black_user = owner_id; game.black_time = main_time_seconds; game.white_time = main_time_seconds; game.black_byo_remaining = byo_yomi_seconds; game.white_byo_remaining = byo_yomi_seconds; game.byo_yomi_seconds = byo_yomi_seconds; game.game_id = game_id; return code;
}

std::string GameService::create_ai_room(int owner_id, int board_size, int main_time_seconds, int byo_yomi_seconds) {
    const auto code = create_room(owner_id, board_size, false, main_time_seconds, byo_yomi_seconds);
    std::lock_guard lock(mutex_); auto& game = require_game(code); game.ai_white = true;
    { std::lock_guard database_lock(database_.mutex()); sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "UPDATE games SET ai_side='white' WHERE id=?", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, game.game_id); sqlite3_step(statement); sqlite3_finalize(statement); }
    return code;
}

std::vector<RoomSummary> GameService::public_rooms() const {
    std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{};
    sqlite3_prepare_v2(database_.handle(), "SELECT room_code,board_size,is_public,status,owner_id FROM rooms WHERE is_public=1 AND status IN ('waiting','playing') ORDER BY id DESC", -1, &statement, nullptr);
    std::vector<RoomSummary> rooms; while (sqlite3_step(statement) == SQLITE_ROW) rooms.push_back({reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)), sqlite3_column_int(statement, 1), sqlite3_column_int(statement, 2) != 0, reinterpret_cast<const char*>(sqlite3_column_text(statement, 3)), sqlite3_column_int(statement, 4)}); sqlite3_finalize(statement); return rooms;
}

GameService::ActiveGame& GameService::require_game(const std::string& room_code) { const auto it = active_games_.find(room_code); return it == active_games_.end() ? load_game(room_code) : it->second; }

GameService::ActiveGame& GameService::load_game(const std::string& room_code) {
    int game_id{}, size{}, black{}, white{}, black_clock{}, white_clock{}, black_byo{}, white_byo{}, passes{}, byo{}; long long turn_epoch{}; bool finished{}, ai_white{};
    std::vector<SgfMove> moves;
    { std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{};
      sqlite3_prepare_v2(database_.handle(), "SELECT g.id,r.board_size,g.black_user_id,COALESCE(g.white_user_id,0),g.black_time_seconds,g.white_time_seconds,g.black_byo_remaining,g.white_byo_remaining,g.consecutive_passes,r.byo_yomi_seconds,g.turn_started_epoch,g.ended_at IS NOT NULL,COALESCE(g.ai_side,'') FROM games g JOIN rooms r ON r.id=g.room_id WHERE r.room_code=? ORDER BY g.id DESC LIMIT 1", -1, &statement, nullptr); bind_text(statement, 1, room_code);
      if (sqlite3_step(statement) != SQLITE_ROW) { sqlite3_finalize(statement); throw std::invalid_argument("房间不存在"); }
      game_id=sqlite3_column_int(statement,0); size=sqlite3_column_int(statement,1); black=sqlite3_column_int(statement,2); white=sqlite3_column_int(statement,3); black_clock=sqlite3_column_int(statement,4); white_clock=sqlite3_column_int(statement,5); black_byo=sqlite3_column_int(statement,6); white_byo=sqlite3_column_int(statement,7); passes=sqlite3_column_int(statement,8); byo=sqlite3_column_int(statement,9); turn_epoch=sqlite3_column_int64(statement,10); finished=sqlite3_column_int(statement,11)!=0; ai_white=std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement,12)))=="white"; sqlite3_finalize(statement);
      sqlite3_prepare_v2(database_.handle(), "SELECT color,x,y FROM moves WHERE game_id=? ORDER BY move_number", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, game_id);
      while (sqlite3_step(statement) == SQLITE_ROW) { const auto color = std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement,0))) == "black" ? Stone::black : Stone::white; const int x = sqlite3_column_type(statement,1)==SQLITE_NULL ? -1 : sqlite3_column_int(statement,1); const int y = sqlite3_column_type(statement,2)==SQLITE_NULL ? -1 : sqlite3_column_int(statement,2); moves.push_back({color,x,y}); } sqlite3_finalize(statement);
    }
    auto [it, inserted] = active_games_.emplace(std::piecewise_construct, std::forward_as_tuple(room_code), std::forward_as_tuple(size)); auto& game=it->second;
    game.game_id=game_id; game.black_user=black; game.white_user=white; game.black_time=black_clock; game.white_time=white_clock; game.black_byo_remaining=black_byo > 0 ? black_byo : byo; game.white_byo_remaining=white_byo > 0 ? white_byo : byo; game.byo_yomi_seconds=byo; game.passes=passes; game.finished=finished; game.ai_white=ai_white;
    for (const auto& move : moves) { if (game.board.current_player()!=move.color) throw std::runtime_error("数据库棋谱顺序损坏"); const auto result = move.x < 0 ? MoveResult{true,{ },0} : game.board.play(move.x,move.y); if (move.x < 0) game.board.pass(); if (!result.accepted) throw std::runtime_error("数据库棋谱包含非法着法"); game.moves.push_back(move); }
    game.sequence=static_cast<int>(moves.size());
    const auto now_epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    const bool timed_out = !game.finished && game.white_user != 0 && turn_epoch > 0 && now_epoch > turn_epoch && !charge_elapsed(game, static_cast<int>(now_epoch - turn_epoch));
    if (timed_out) finish_game(room_code, game, game.board.current_player() == Stone::black ? "W+T" : "B+T", "timeout");
    game.turn_started=std::chrono::steady_clock::now(); persist_clock(game); return game;
}

GameSnapshot GameService::snapshot(const std::string& code, const ActiveGame& game) {
    GameSnapshot result{code, game.board.size(), game.board.current_player(), {}, game.black_time, game.white_time, game.black_byo_remaining, game.white_byo_remaining, game.byo_yomi_seconds, game.passes, game.finished, game.sequence, game.black_user, game.white_user, game.ai_white};
    result.stones.reserve(game.board.size() * game.board.size()); for (int y = 0; y < game.board.size(); ++y) for (int x = 0; x < game.board.size(); ++x) result.stones.push_back(static_cast<int>(game.board.at(x, y))); return result;
}

GameSnapshot GameService::join_room(int user_id, const std::string& room_code, bool spectate) {
    std::lock_guard lock(mutex_); auto& game = require_game(room_code);
    if (!spectate && game.ai_white && user_id != game.black_user) throw std::invalid_argument("这是人机对局，只能以观战身份进入");
    if (!spectate && game.white_user == 0 && !game.ai_white && user_id != game.black_user) {
        game.white_user = user_id; std::lock_guard database_lock(database_.mutex()); sqlite3_stmt* statement{};
        sqlite3_prepare_v2(database_.handle(), "UPDATE games SET white_user_id=? WHERE id=?", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, user_id); sqlite3_bind_int(statement, 2, game.game_id); sqlite3_step(statement); sqlite3_finalize(statement);
        sqlite3_prepare_v2(database_.handle(), "UPDATE rooms SET status='playing' WHERE room_code=?", -1, &statement, nullptr); bind_text(statement, 1, room_code); sqlite3_step(statement); sqlite3_finalize(statement);
    } else if (!spectate && user_id != game.black_user && user_id != game.white_user) throw std::invalid_argument("房间已满，请以观战身份进入");
    const auto state = snapshot(room_code, game); emit({"room.snapshot", room_code, user_id, state}); return state;
}

void GameService::leave_room(int user_id, const std::string& room_code) { std::lock_guard lock(mutex_); auto& game = require_game(room_code); if (!game.finished && (game.black_user == user_id || game.white_user == user_id)) emit({"game.updated", room_code, std::nullopt, snapshot(room_code, game), "对手暂时断线"}); }

void GameService::persist_move(const ActiveGame& game, const SgfMove& move, int captured) {
    std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "INSERT INTO moves(game_id,move_number,color,x,y,captured) VALUES(?,?,?,?,?,?)", -1, &statement, nullptr);
    sqlite3_bind_int(statement, 1, game.game_id); sqlite3_bind_int(statement, 2, static_cast<int>(game.moves.size())); sqlite3_bind_text(statement, 3, move.color == Stone::black ? "black" : "white", -1, SQLITE_STATIC);
    if (move.x < 0) sqlite3_bind_null(statement, 4); else sqlite3_bind_int(statement, 4, move.x); if (move.y < 0) sqlite3_bind_null(statement, 5); else sqlite3_bind_int(statement, 5, move.y); sqlite3_bind_int(statement, 6, captured);
    const int result = sqlite3_step(statement); sqlite3_finalize(statement); if (result != SQLITE_DONE) throw std::runtime_error("无法保存着法");
}

void GameService::persist_clock(const ActiveGame& game) {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "UPDATE games SET black_time_seconds=?,white_time_seconds=?,black_byo_remaining=?,white_byo_remaining=?,consecutive_passes=?,turn_started_epoch=strftime('%s','now') WHERE id=?", -1, &statement, nullptr);
    sqlite3_bind_int(statement,1,game.black_time); sqlite3_bind_int(statement,2,game.white_time); sqlite3_bind_int(statement,3,game.black_byo_remaining); sqlite3_bind_int(statement,4,game.white_byo_remaining); sqlite3_bind_int(statement,5,game.passes); sqlite3_bind_int(statement,6,game.game_id); sqlite3_step(statement); sqlite3_finalize(statement);
}

void GameService::start_next_turn(ActiveGame& game) {
    if (game.board.current_player() == Stone::black && game.black_time <= 0) game.black_byo_remaining = game.byo_yomi_seconds;
    if (game.board.current_player() == Stone::white && game.white_time <= 0) game.white_byo_remaining = game.byo_yomi_seconds;
    game.turn_started = std::chrono::steady_clock::now();
}

bool GameService::charge_elapsed(ActiveGame& game, int elapsed_seconds) {
    int& main = game.board.current_player() == Stone::black ? game.black_time : game.white_time;
    int& byo = game.board.current_player() == Stone::black ? game.black_byo_remaining : game.white_byo_remaining;
    if (elapsed_seconds <= 0) return true;
    if (main > 0) { const int used = std::min(main, elapsed_seconds); main -= used; elapsed_seconds -= used; }
    if (elapsed_seconds > 0) byo -= elapsed_seconds;
    return main > 0 || byo > 0;
}

void GameService::finish_game(const std::string& room_code, ActiveGame& game, const std::string& result, const std::string& reason) {
    if (game.finished) return; game.finished = true; ++game.sequence; const auto sgf = make_sgf(game.board.size(), 7.5, game.moves, result);
    std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "UPDATE games SET result=?,finish_reason=?,ended_at=CURRENT_TIMESTAMP,sgf=? WHERE id=?", -1, &statement, nullptr);
    bind_text(statement, 1, result); bind_text(statement, 2, reason); bind_text(statement, 3, sgf); sqlite3_bind_int(statement, 4, game.game_id); sqlite3_step(statement); sqlite3_finalize(statement);
    sqlite3_prepare_v2(database_.handle(), "UPDATE rooms SET status='finished' WHERE room_code=?", -1, &statement, nullptr); bind_text(statement, 1, room_code); sqlite3_step(statement); sqlite3_finalize(statement);
}

GameSnapshot GameService::move(int user_id, const std::string& room_code, int x, int y) {
    std::lock_guard lock(mutex_); auto& game = require_game(room_code); if (game.finished) throw std::invalid_argument("对局已经结束"); const int expected = game.board.current_player() == Stone::black ? game.black_user : game.white_user; if (expected == 0 || expected != user_id) throw std::invalid_argument("当前无权落子");
    const auto color = game.board.current_player(); const auto result = game.board.play(x, y); if (!result.accepted) throw std::invalid_argument(result.error); game.moves.push_back({color, x, y}); persist_move(game, game.moves.back(), result.captured); game.passes = 0; ++game.sequence; start_next_turn(game); persist_clock(game);
    const auto state = snapshot(room_code, game); emit({"game.updated", room_code, std::nullopt, state}); return state;
}

GameSnapshot GameService::pass(int user_id, const std::string& room_code) {
    std::lock_guard lock(mutex_); auto& game = require_game(room_code); const int expected = game.board.current_player() == Stone::black ? game.black_user : game.white_user; if (game.finished || expected == 0 || expected != user_id) throw std::invalid_argument("当前无权停一手");
    const auto color = game.board.current_player(); game.board.pass(); game.moves.push_back({color, -1, -1}); persist_move(game, game.moves.back(), 0); ++game.passes; ++game.sequence; start_next_turn(game); persist_clock(game);
    if (game.passes >= 2) { const auto score = game.board.chinese_area_score(); finish_game(room_code, game, score_result(score), "two_passes"); }
    const auto state = snapshot(room_code, game); emit({game.finished ? "game.finished" : "game.updated", room_code, std::nullopt, state}); return state;
}

GameSnapshot GameService::resign(int user_id, const std::string& room_code) { std::lock_guard lock(mutex_); auto& game = require_game(room_code); if (user_id != game.black_user && user_id != game.white_user) throw std::invalid_argument("观战者不能认输"); finish_game(room_code, game, user_id == game.black_user ? "W+R" : "B+R", "resign"); const auto state = snapshot(room_code, game); emit({"game.finished", room_code, std::nullopt, state}); return state; }

bool GameService::ai_turn(const std::string& room_code) { std::lock_guard lock(mutex_); const auto& game=require_game(room_code); return game.ai_white && !game.finished && game.board.current_player()==Stone::white; }
GameSnapshot GameService::ai_move(const std::string& room_code, int x, int y) {
    std::lock_guard lock(mutex_); auto& game=require_game(room_code); if(!game.ai_white || game.finished || game.board.current_player()!=Stone::white) throw std::invalid_argument("当前不是 KataGo 回合");
    const auto result=game.board.play(x,y); if(!result.accepted) throw std::invalid_argument("KataGo 返回了非法着法："+result.error); game.moves.push_back({Stone::white,x,y}); persist_move(game,game.moves.back(),result.captured); game.passes=0; ++game.sequence; start_next_turn(game); persist_clock(game); const auto state=snapshot(room_code,game); emit({"game.updated",room_code,std::nullopt,state}); return state;
}

GameSnapshot GameService::ai_pass(const std::string& room_code) {
    std::lock_guard lock(mutex_); auto& game=require_game(room_code); if(!game.ai_white || game.finished || game.board.current_player()!=Stone::white) throw std::invalid_argument("当前不是 KataGo 回合");
    game.board.pass(); game.moves.push_back({Stone::white,-1,-1}); persist_move(game,game.moves.back(),0); ++game.passes; ++game.sequence; start_next_turn(game); persist_clock(game);
    if (game.passes >= 2) { const auto score=game.board.chinese_area_score(); finish_game(room_code,game,score_result(score),"two_passes"); }
    const auto state=snapshot(room_code,game); emit({game.finished ? "game.finished" : "game.updated",room_code,std::nullopt,state}); return state;
}

std::vector<SgfMove> GameService::moves_for_room(const std::string& room_code) {
    std::lock_guard lock(mutex_); return require_game(room_code).moves;
}

std::optional<GameSnapshot> GameService::join_matchmaking(int user_id, int board_size) {
    if (board_size != 9 && board_size != 13 && board_size != 19) throw std::invalid_argument("棋盘大小无效"); int opponent{};
    { std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "SELECT user_id FROM matchmaking_queue WHERE board_size=? AND user_id<>? ORDER BY joined_at LIMIT 1", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, board_size); sqlite3_bind_int(statement, 2, user_id); if (sqlite3_step(statement) == SQLITE_ROW) opponent = sqlite3_column_int(statement, 0); sqlite3_finalize(statement);
      if (opponent == 0) { sqlite3_prepare_v2(database_.handle(), "INSERT INTO matchmaking_queue(user_id,board_size) VALUES(?,?) ON CONFLICT(user_id) DO UPDATE SET board_size=excluded.board_size,joined_at=CURRENT_TIMESTAMP", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, user_id); sqlite3_bind_int(statement, 2, board_size); sqlite3_step(statement); sqlite3_finalize(statement); return std::nullopt; }
      sqlite3_prepare_v2(database_.handle(), "DELETE FROM matchmaking_queue WHERE user_id IN (?,?)", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, user_id); sqlite3_bind_int(statement, 2, opponent); sqlite3_step(statement); sqlite3_finalize(statement); }
    const auto code = create_room(opponent, board_size, false); const auto state = join_room(user_id, code, false); emit({"match.found", code, opponent, state}); emit({"match.found", code, user_id, state}); return state;
}

void GameService::cancel_matchmaking(int user_id) { std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "DELETE FROM matchmaking_queue WHERE user_id=?", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, user_id); sqlite3_step(statement); sqlite3_finalize(statement); }

void GameService::send_chat(int user_id, const std::string& nickname, const std::string& room_code, const std::string& content) {
    if (content.empty() || content.size() > 500) throw std::invalid_argument("聊天内容须为 1–500 个字符"); { std::lock_guard lock(mutex_); require_game(room_code); }
    { std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "INSERT INTO chat_messages(room_id,user_id,content) SELECT id,?,? FROM rooms WHERE room_code=?", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, user_id); bind_text(statement, 2, content); bind_text(statement, 3, room_code); if (sqlite3_step(statement) != SQLITE_DONE) { sqlite3_finalize(statement); throw std::runtime_error("无法保存聊天记录"); } sqlite3_finalize(statement); }
    emit({"chat.message", room_code, std::nullopt, {}, content, nickname});
}

std::vector<ChatMessage> GameService::chat_history(int user_id, const std::string& room_code) const {
    std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{};
    sqlite3_prepare_v2(database_.handle(), "SELECT u.nickname,c.content,c.created_at FROM chat_messages c JOIN rooms r ON r.id=c.room_id JOIN users u ON u.id=c.user_id WHERE r.room_code=? ORDER BY c.id DESC LIMIT 100", -1, &statement, nullptr);
    bind_text(statement,1,room_code); (void)user_id; std::vector<ChatMessage> messages;
    while(sqlite3_step(statement)==SQLITE_ROW) { const auto nickname=reinterpret_cast<const char*>(sqlite3_column_text(statement,0)); const auto content=reinterpret_cast<const char*>(sqlite3_column_text(statement,1)); const auto created=reinterpret_cast<const char*>(sqlite3_column_text(statement,2)); messages.push_back({nickname?nickname:"",content?content:"",created?created:""}); }
    sqlite3_finalize(statement); std::reverse(messages.begin(),messages.end()); return messages;
}

std::vector<GameRecordSummary> GameService::game_records(int user_id) const { std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "SELECT id,COALESCE(result,''),COALESCE(finish_reason,''),COALESCE(ended_at,'') FROM games WHERE black_user_id=? OR white_user_id=? ORDER BY id DESC", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, user_id); sqlite3_bind_int(statement, 2, user_id); std::vector<GameRecordSummary> records; while (sqlite3_step(statement) == SQLITE_ROW) records.push_back({sqlite3_column_int(statement, 0), reinterpret_cast<const char*>(sqlite3_column_text(statement, 1)), reinterpret_cast<const char*>(sqlite3_column_text(statement, 2)), reinterpret_cast<const char*>(sqlite3_column_text(statement, 3))}); sqlite3_finalize(statement); return records; }

std::string GameService::game_sgf(int user_id, int game_id) const { std::lock_guard lock(database_.mutex()); sqlite3_stmt* statement{}; sqlite3_prepare_v2(database_.handle(), "SELECT COALESCE(sgf,'') FROM games WHERE id=? AND (black_user_id=? OR white_user_id=?)", -1, &statement, nullptr); sqlite3_bind_int(statement, 1, game_id); sqlite3_bind_int(statement, 2, user_id); sqlite3_bind_int(statement, 3, user_id); if (sqlite3_step(statement) != SQLITE_ROW) { sqlite3_finalize(statement); throw std::invalid_argument("棋谱不存在或无权访问"); } const auto text = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)); std::string sgf = text ? text : ""; sqlite3_finalize(statement); return sgf; }

void GameService::tick() { std::lock_guard lock(mutex_); const auto now = std::chrono::steady_clock::now(); for (auto& [code, game] : active_games_) { if (game.finished || (game.white_user == 0 && !game.ai_white)) continue; const int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(now - game.turn_started).count()); if (elapsed <= 0) continue; game.turn_started = now; const bool alive=charge_elapsed(game,elapsed); ++game.sequence; persist_clock(game); if (!alive) { finish_game(code, game, game.board.current_player() == Stone::black ? "W+T" : "B+T", "timeout"); emit({"game.finished", code, std::nullopt, snapshot(code, game)}); } else emit({"clock.updated", code, std::nullopt, snapshot(code, game)}); } }
void GameService::set_event_listener(std::function<void(const RealtimeEvent&)> listener) { std::lock_guard lock(mutex_); event_listener_ = std::move(listener); }
void GameService::emit(RealtimeEvent event) const { if (event_listener_) event_listener_(event); }

} // namespace weiqi
