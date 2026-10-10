#pragma once

#include "weiqi/sgf.hpp"
#include <string>
#include <vector>

namespace weiqi {
struct ReplayFrame {
    std::vector<int> stones;
    Stone current_player{};
    int last_x{-1};
    int last_y{-1};
};
struct Replay {
    int board_size{};
    double komi{};
    std::string result;
    std::vector<SgfMove> moves;
    std::vector<ReplayFrame> frames;
};
// Replay legality, captures and resulting positions are computed in C++.
[[nodiscard]] Replay build_replay(const std::string &sgf);
} // namespace weiqi
