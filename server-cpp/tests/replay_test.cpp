#include "weiqi/replay.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <stdexcept>

int main() {
    const auto replay = weiqi::build_replay(
        R"SGF((;GM[1]FF[4]SZ[9]KM[7.5]RE[B+R]C[escaped \] and fake ;W[ii\]];B[ba];W[bb];B[ab];W[ii];B[cb];W[ih];B[bc];W[];B[]))SGF");
    assert(replay.board_size == 9 && replay.komi == 7.5 && replay.result == "B+R");
    assert(replay.moves.size() == 9 && replay.frames.size() == 10);
    assert(replay.frames[0].stones[10] == 0);
    assert(replay.frames[2].stones[10] == 2);
    assert(replay.frames[7].stones[10] == 0); // white group captured by C++ rules
    assert(replay.frames[7].last_x == 1 && replay.frames[7].last_y == 2);
    assert(replay.frames[8].stones == replay.frames[7].stones && replay.frames[8].last_x == -1);
    for (const auto *invalid :
         {"", "(;SZ[10])", "(;SZ[9];B[zz])", "(;SZ[9];B[aa];W[aa])", "(;SZ[9];W[aa])", "(;SZ[9];B[aa];B[bb])",
          "(;SZ[9];B[aa]", "(;SZ[9]C[broken)", "(;SZ[9]AB[aa])", "(;SZ[9];B[aa](;W[bb])(;W[cc]))",
          "(;SZ[9])(;SZ[9])", "(;GM[2]SZ[9])", "(;SZ[9]KM[nan])"}) {
        bool rejected = false;
        try {
            (void)weiqi::build_replay(invalid);
        } catch (const std::exception &) {
            rejected = true;
        }
        assert(rejected);
    }
    assert(weiqi::build_replay("\n ( ; SZ[19] KM[6.5] ;B[ss] ; W[] ) \n").frames.size() == 3);
}
