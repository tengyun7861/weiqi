#include "weiqi/replay.hpp"
#include <stdexcept>

namespace weiqi {
Replay build_replay(const std::string &sgf) {
    const auto document = parse_sgf_document(sgf);
    if (document.moves.size() > 10000)
        throw std::invalid_argument("棋谱着法过多");
    Replay result{document.board_size, document.komi, document.result, document.moves, {}};
    GoBoard board(document.board_size);
    auto save = [&](int x, int y) {
        ReplayFrame frame{{}, board.current_player(), x, y};
        frame.stones.reserve(board.size() * board.size());
        for (int row = 0; row < board.size(); ++row)
            for (int column = 0; column < board.size(); ++column)
                frame.stones.push_back(static_cast<int>(board.at(column, row)));
        result.frames.push_back(std::move(frame));
    };
    save(-1, -1);
    for (const auto &move : document.moves) {
        if (move.color != board.current_player())
            throw std::invalid_argument("棋谱行棋顺序无效");
        if (move.x < 0)
            board.pass();
        else {
            const auto played = board.play(move.x, move.y);
            if (!played.accepted)
                throw std::invalid_argument("棋谱包含非法着法：" + played.error);
        }
        save(move.x, move.y);
    }
    return result;
}
} // namespace weiqi
