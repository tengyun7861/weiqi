#pragma once

#include "weiqi/database.hpp"
#include "weiqi/go_board.hpp"
#include "weiqi/sgf.hpp"

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <functional>
#include <vector>

namespace weiqi {

struct RoomSummary { std::string code; int board_size{}; bool is_public{}; std::string status; int owner_id{}; };
struct GameSnapshot { std::string room_code; int board_size{}; Stone current_player{}; std::vector<int> stones; int black_time{}; int white_time{}; int black_byo_remaining{}; int white_byo_remaining{}; int byo_yomi_seconds{}; int consecutive_passes{}; bool finished{}; int sequence{}; int black_user{}; int white_user{}; bool ai_white{}; };
struct GameRecordSummary { int id{}; std::string result; std::string finish_reason; std::string ended_at; };
struct ChatMessage { std::string nickname; std::string content; std::string created_at; };
struct RealtimeEvent { std::string type; std::string room_code; std::optional<int> user_id; GameSnapshot snapshot; std::string message; std::string nickname; };

class GameService final {
public:
    explicit GameService(Database& database) : database_(database) {}
    [[nodiscard]] std::string create_room(int owner_id, int board_size, bool is_public, int main_time_seconds = 600, int byo_yomi_seconds = 30);
    [[nodiscard]] std::string create_ai_room(int owner_id, int board_size, int main_time_seconds = 600, int byo_yomi_seconds = 30);
    [[nodiscard]] std::vector<RoomSummary> public_rooms() const;
    [[nodiscard]] GameSnapshot join_room(int user_id, const std::string& room_code, bool spectate);
    void leave_room(int user_id, const std::string& room_code);
    [[nodiscard]] GameSnapshot move(int user_id, const std::string& room_code, int x, int y);
    [[nodiscard]] GameSnapshot pass(int user_id, const std::string& room_code);
    [[nodiscard]] GameSnapshot resign(int user_id, const std::string& room_code);
    [[nodiscard]] GameSnapshot ai_move(const std::string& room_code, int x, int y);
    [[nodiscard]] bool ai_turn(const std::string& room_code);
    [[nodiscard]] GameSnapshot ai_pass(const std::string& room_code);
    [[nodiscard]] std::vector<SgfMove> moves_for_room(const std::string& room_code);
    [[nodiscard]] std::optional<GameSnapshot> join_matchmaking(int user_id, int board_size);
    void cancel_matchmaking(int user_id);
    void send_chat(int user_id, const std::string& nickname, const std::string& room_code, const std::string& content);
    [[nodiscard]] std::vector<ChatMessage> chat_history(int user_id, const std::string& room_code) const;
    [[nodiscard]] std::vector<GameRecordSummary> game_records(int user_id) const;
    [[nodiscard]] std::string game_sgf(int user_id, int game_id) const;
    void tick();
    void set_event_listener(std::function<void(const RealtimeEvent&)> listener);

private:
    struct ActiveGame {
        explicit ActiveGame(int size) : board(size) {}
        GoBoard board;
        int black_user{};
        int white_user{};
        int black_time{};
        int white_time{};
        int black_byo_remaining{};
        int white_byo_remaining{};
        int byo_yomi_seconds{};
        int passes{};
        bool finished{};
        bool ai_white{};
        int game_id{};
        int sequence{};
        std::chrono::steady_clock::time_point turn_started{std::chrono::steady_clock::now()};
        std::vector<SgfMove> moves;
    };
    ActiveGame& require_game(const std::string& room_code);
    ActiveGame& load_game(const std::string& room_code);
    [[nodiscard]] static GameSnapshot snapshot(const std::string& code, const ActiveGame& game);
    void persist_move(const ActiveGame& game, const SgfMove& move, int captured);
    void persist_clock(const ActiveGame& game);
    void start_next_turn(ActiveGame& game);
    [[nodiscard]] bool charge_elapsed(ActiveGame& game, int elapsed_seconds);
    void finish_game(const std::string& room_code, ActiveGame& game, const std::string& result, const std::string& reason);
    void emit(RealtimeEvent event) const;
    Database& database_;
    mutable std::mutex mutex_;
    std::map<std::string, ActiveGame> active_games_;
    std::function<void(const RealtimeEvent&)> event_listener_;
};

} // namespace weiqi
