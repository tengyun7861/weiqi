#include "weiqi/ai_worker.hpp"
#include <stdexcept>

namespace weiqi {
AiWorker::AiWorker(GameService &games, KataGoGtp &engine)
    : games_(games), engine_(engine), thread_([this](std::stop_token stop) { run(stop); }) {}
AiWorker::~AiWorker() {
    thread_.request_stop();
    ready_.notify_all();
}
void AiWorker::schedule(const std::string &room_code) {
    std::lock_guard lock(mutex_);
    if (pending_.insert(room_code).second) {
        queue_.push_back(room_code);
        ready_.notify_one();
    }
}
void AiWorker::run(std::stop_token stop) {
    while (!stop.stop_requested()) {
        std::string room;
        {
            std::unique_lock lock(mutex_);
            if (!ready_.wait(lock, stop, [&] { return !queue_.empty(); }))
                return;
            room = queue_.front();
            queue_.pop_front();
        }
        int starting_move_count = -1;
        bool applied = false;
        try {
            if (games_.ai_turn(room)) {
                const auto state = games_.room_state(room);
                starting_move_count = state.move_count;
                if (!engine_.new_game(state.board_size))
                    throw std::runtime_error(engine_.status());
                for (const auto &move : games_.moves_for_room(room))
                    if (!engine_.play(move.color, move.x, move.y, state.board_size))
                        throw std::runtime_error(engine_.status());
                const auto answer = engine_.generate_move(Stone::white, state.board_size);
                if (games_.ai_turn(room)) {
                    if (answer.kind == GtpMoveKind::place) {
                        (void)games_.ai_move(room, answer.x, answer.y);
                        applied = true;
                    } else if (answer.kind == GtpMoveKind::pass) {
                        (void)games_.ai_pass(room);
                        applied = true;
                    } else if (answer.kind == GtpMoveKind::resign)
                        (void)games_.ai_resign(room);
                    else
                        games_.ai_error(room, answer.error);
                }
            }
        } catch (const std::exception &error) {
            try {
                games_.ai_error(room, error.what());
            } catch (...) {
            }
        }
        {
            std::lock_guard lock(mutex_);
            pending_.erase(room);
        }
        // A fast human reply can arrive while the completed job is still marked pending.
        // Queue that new turn without retrying failed inference indefinitely.
        if (applied && !stop.stop_requested()) {
            try {
                const auto current = games_.room_state(room);
                if (current.ai_white && !current.finished && current.current_player == Stone::white &&
                    current.move_count > starting_move_count + 1)
                    schedule(room);
            } catch (const std::exception &error) {
                try {
                    games_.ai_error(room, error.what());
                } catch (...) {
                }
            }
        }
    }
}
} // namespace weiqi
