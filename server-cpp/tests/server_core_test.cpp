#include "weiqi/auth.hpp"
#include "weiqi/database.hpp"
#include "weiqi/game_service.hpp"
#include "weiqi/sgf.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <filesystem>
#include <thread>

int main() {
    const auto database_file = std::filesystem::temp_directory_path() / "weiqi_server_core_test.sqlite3";
    std::filesystem::remove(database_file);
    {
    weiqi::Database database(database_file.string()); database.migrate();
    weiqi::AuthService auth(database); auth.register_user("测试棋手", "correct-horse-battery-staple");
    auth.register_user("对手棋手", "another-correct-horse-battery-staple");
    const auto token = auth.login("测试棋手", "correct-horse-battery-staple");
    const auto user = auth.authenticate(token); assert(user && user->nickname == "测试棋手");
    const auto opponent = auth.authenticate(auth.login("对手棋手", "another-correct-horse-battery-staple")); assert(opponent);
    assert(!auth.authenticate("not-a-token"));
    std::string room;
    {
        weiqi::GameService games(database); room = games.create_room(user->id, 9, true, 300, 20);
        assert(room.size() == 6); assert(games.public_rooms().size() == 1);
        assert(games.join_room(user->id, room, false).board_size == 9);
        assert(games.join_room(opponent->id, room, false).white_user == opponent->id);
        assert(games.move(user->id, room, 0, 0).stones[0] == 1);
    }
    weiqi::GameService recovered(database);
    const auto snapshot = recovered.join_room(user->id, room, false);
    assert(snapshot.board_size == 9 && snapshot.stones[0] == 1);
    assert(!recovered.pass(opponent->id, room).finished);
    assert(recovered.pass(user->id, room).finished);
    const auto records = recovered.game_records(user->id);
    assert(!records.empty() && records.front().result == "B+73.5" && !recovered.game_sgf(user->id, records.front().id).empty());
    const auto ai_room = recovered.create_ai_room(user->id, 9, 300, 20);
    assert(recovered.move(user->id, ai_room, 0, 0).current_player == weiqi::Stone::white);
    assert(recovered.ai_turn(ai_room));
    bool ai_room_rejected{}; try { (void)recovered.join_room(opponent->id, ai_room, false); } catch (const std::invalid_argument&) { ai_room_rejected=true; } assert(ai_room_rejected);
    recovered.send_chat(user->id, user->nickname, ai_room, "重连后仍应看到这条消息");
    const auto chat = recovered.chat_history(user->id, ai_room); assert(chat.size() == 1 && chat.front().content == "重连后仍应看到这条消息");
    weiqi::GameService recovered_ai(database); assert(recovered_ai.ai_turn(ai_room));
    const auto byo_room = recovered.create_room(user->id, 9, false, 0, 1);
    (void)recovered.join_room(opponent->id, byo_room, false);
    std::this_thread::sleep_for(std::chrono::milliseconds(1200)); recovered.tick();
    assert(recovered.join_room(user->id, byo_room, false).finished);
    const std::vector<weiqi::SgfMove> moves{{weiqi::Stone::black, 3, 3}, {weiqi::Stone::white, -1, -1}};
    const auto sgf = weiqi::make_sgf(9, 7.5, moves); assert(weiqi::parse_sgf_moves(sgf, 9).size() == 2);
    }
    std::filesystem::remove(database_file);
}
