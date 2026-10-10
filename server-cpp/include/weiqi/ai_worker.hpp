#pragma once
#include "weiqi/game_service.hpp"
#include "weiqi/katago_gtp.hpp"
#include <condition_variable>
#include <deque>
#include <set>
#include <thread>

namespace weiqi {
// One complete engine task runs at a time, away from HTTP/WebSocket handlers.
class AiWorker final {
  public:
    AiWorker(GameService &games, KataGoGtp &engine);
    ~AiWorker();
    void schedule(const std::string &room_code);

  private:
    void run(std::stop_token stop);
    GameService &games_;
    KataGoGtp &engine_;
    std::mutex mutex_;
    std::condition_variable_any ready_;
    std::deque<std::string> queue_;
    std::set<std::string> pending_;
    std::jthread thread_;
};
} // namespace weiqi
