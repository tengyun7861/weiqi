#include "weiqi/go_board.hpp"
#include "weiqi/sgf.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <stdexcept>

using weiqi::GoBoard;
using weiqi::Stone;

int main() {
    GoBoard board(9);
    assert(board.current_player() == Stone::black);
    assert(!board.play(-1, 0).accepted);
    assert(board.play(1, 0).accepted);  // B
    assert(board.play(1, 1).accepted);  // W
    assert(board.play(0, 1).accepted);  // B
    assert(board.play(8, 8).accepted);  // W
    assert(board.play(2, 1).accepted);  // B
    assert(board.play(8, 7).accepted);  // W
    const auto capture = board.play(1, 2); // B captures the white stone
    assert(capture.accepted && capture.captured == 1);
    assert(board.at(1, 1) == Stone::empty);
    assert(!board.play(1, 0).accepted);

    bool rejected_size = false;
    try { GoBoard invalid(10); } catch (const std::invalid_argument&) { rejected_size = true; }
    assert(rejected_size);

    const auto sgf = weiqi::make_sgf(9, 7.5, {{Stone::black, 0, 0}, {Stone::white, -1, -1}});
    const auto moves = weiqi::parse_sgf_moves(sgf, 9);
    assert(moves.size() == 2 && moves[0].x == 0 && moves[1].x == -1);
    const auto area = board.chinese_area_score();
    assert(area.black > 0 && area.white > 0);
}
