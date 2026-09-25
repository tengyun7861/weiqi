#pragma once

#include "weiqi/go_board.hpp"

#include <mutex>
#include <optional>
#include <string>

namespace weiqi {

// Owns the local KataGo GTP subprocess. This is deliberately server-only:
// clients submit an intent and never receive an engine command line or model path.
class KataGoGtp final {
public:
    KataGoGtp();
    ~KataGoGtp();
    KataGoGtp(const KataGoGtp&) = delete;
    KataGoGtp& operator=(const KataGoGtp&) = delete;

    [[nodiscard]] bool available() const noexcept;
    [[nodiscard]] std::string status();
    bool new_game(int board_size);
    bool play(Stone color, int x, int y, int board_size);
    [[nodiscard]] std::optional<std::pair<int, int>> genmove(Stone color, int board_size);
    void stop() noexcept;

private:
    bool start_locked();
    [[nodiscard]] std::optional<std::string> command_locked(const std::string& command);
    [[nodiscard]] static std::string vertex(int x, int y, int board_size);
    std::string command_line_;
    std::string reason_;
    mutable std::mutex mutex_;
    void* process_{};
    void* stdin_write_{};
    void* stdout_read_{};
};

} // namespace weiqi
