#pragma once
#include "weiqi/go_board.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

namespace weiqi::detail {
inline std::string score_result(const AreaScore &score) {
    const double margin = static_cast<double>(score.black) - (static_cast<double>(score.white) + 7.5);
    std::ostringstream text;
    text << std::fixed << std::setprecision(1) << std::abs(margin);
    auto result = text.str();
    if (result.ends_with(".0"))
        result.resize(result.size() - 2);
    return std::string(margin > 0 ? "B+" : "W+") + result;
}
} // namespace weiqi::detail
