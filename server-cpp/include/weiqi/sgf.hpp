#pragma once

#include "weiqi/go_board.hpp"

#include <string>
#include <vector>

namespace weiqi {

struct SgfMove {
    Stone color{Stone::black};
    int x{-1};
    int y{-1};
};

[[nodiscard]] std::string make_sgf(int board_size, double komi, const std::vector<SgfMove>& moves,
                                   const std::string& result = "");
[[nodiscard]] std::vector<SgfMove> parse_sgf_moves(const std::string& sgf, int expected_board_size);

} // namespace weiqi
