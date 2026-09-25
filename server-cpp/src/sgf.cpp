#include "weiqi/sgf.hpp"

#include <regex>
#include <stdexcept>

namespace weiqi {

std::string make_sgf(int board_size, double komi, const std::vector<SgfMove>& moves, const std::string& result) {
    std::string sgf = "(;GM[1]FF[4]CA[UTF-8]RU[Chinese]SZ[" + std::to_string(board_size) + "]KM[" + std::to_string(komi) + "]";
    if (!result.empty()) sgf += "RE[" + result + "]";
    for (const auto& move : moves) {
        sgf += ';';
        sgf += move.color == Stone::black ? "B[" : "W[";
        if (move.x >= 0 && move.y >= 0) {
            sgf.push_back(static_cast<char>('a' + move.x));
            sgf.push_back(static_cast<char>('a' + move.y));
        }
        sgf += ']';
    }
    return sgf + ')';
}

std::vector<SgfMove> parse_sgf_moves(const std::string& sgf, int expected_board_size) {
    const std::regex size_pattern(R"(SZ\[(\d+)\])");
    std::smatch size_match;
    if (!std::regex_search(sgf, size_match, size_pattern) || std::stoi(size_match[1]) != expected_board_size) {
        throw std::invalid_argument("SGF 棋盘大小不匹配");
    }
    const std::regex move_pattern(R"(;([BW])\[([a-s]{0,2})\])");
    std::vector<SgfMove> moves;
    for (std::sregex_iterator it(sgf.begin(), sgf.end(), move_pattern), end; it != end; ++it) {
        const auto coordinate = (*it)[2].str();
        SgfMove move{(*it)[1].str() == "B" ? Stone::black : Stone::white, -1, -1};
        if (!coordinate.empty()) {
            if (coordinate.size() != 2) throw std::invalid_argument("SGF 坐标无效");
            move.x = coordinate[0] - 'a';
            move.y = coordinate[1] - 'a';
        }
        moves.push_back(move);
    }
    return moves;
}

} // namespace weiqi
