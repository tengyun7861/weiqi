#include "weiqi/ai_worker.hpp"
#include "weiqi/auth.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <thread>

template <class Predicate> void await(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    while (!predicate() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    assert(predicate());
}

int main(int argc, char *argv[]) {
    assert(argc == 2);
    _putenv_s("WEIQI_KATAGO_COMMAND", argv[1]);
    _putenv_s("WEIQI_GTP_TEST_MOVE", "pass");
    _putenv_s("WEIQI_GTP_TEST_DELAY_MS", "80");
    const auto path = std::filesystem::temp_directory_path() / "weiqi_ai_worker_test.sqlite3";
    std::filesystem::remove(path);
    {
        weiqi::Database database(path.string());
        database.migrate();
        weiqi::AuthService auth(database);
        auth.register_user("AI队列棋手", "ai-worker-test-password");
        const auto user = auth.authenticate(auth.login("AI队列棋手", "ai-worker-test-password"));
        assert(user);
        weiqi::GameService games(database);
        std::atomic_int errors{0};
        games.set_event_listener([&](const weiqi::RealtimeEvent &event) {
            if (event.type == "ai.error")
                ++errors;
        });
        {
            weiqi::KataGoGtp engine;
            assert(engine.status() == "KataGo GTP 已就绪");
            weiqi::AiWorker worker(games, engine);
            const auto small = games.create_ai_room(user->id, 9);
            const auto medium = games.create_ai_room(user->id, 13);
            (void)games.move(user->id, small, 0, 0);
            (void)games.move(user->id, medium, 1, 1);
            worker.schedule(small);
            worker.schedule(medium);
            worker.schedule(small);
            await([&] {
                return games.room_state(small).move_count == 2 && games.room_state(medium).move_count == 2;
            });
            assert(games.room_state(small).stones[0] == 1 && !games.ai_turn(small));
            assert(games.room_state(medium).stones[14] == 1 && !games.ai_turn(medium));
            assert(errors == 0);
            assert(games.pass(user->id, small).finished); // real pass result, not fake placement
        }
        _putenv_s("WEIQI_GTP_TEST_MOVE", "resign");
        {
            weiqi::KataGoGtp engine;
            weiqi::AiWorker worker(games, engine);
            const auto room = games.create_ai_room(user->id, 9);
            (void)games.move(user->id, room, 0, 0);
            worker.schedule(room);
            await([&] { return games.room_state(room).finished; });
            assert(games.room_state(room).result == "B+R");
        }
        _putenv_s("WEIQI_GTP_TEST_MOVE", "I4");
        {
            weiqi::KataGoGtp engine;
            weiqi::AiWorker worker(games, engine);
            const auto room = games.create_ai_room(user->id, 9);
            (void)games.move(user->id, room, 0, 0);
            worker.schedule(room);
            await([&] { return errors > 0; });
            assert(games.ai_turn(room) && games.room_state(room).move_count == 1);
            assert(!games.room_state(room).finished); // failed inference never becomes pass or resign
        }
    }
    std::filesystem::remove(path);
    _putenv_s("WEIQI_GTP_TEST_MOVE", "");
    _putenv_s("WEIQI_GTP_TEST_DELAY_MS", "");
}
