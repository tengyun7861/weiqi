#include "weiqi/katago_gtp.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdlib>

int main(int argc, char* argv[]) {
    assert(argc == 2);
    _putenv_s("WEIQI_KATAGO_COMMAND", argv[1]);
    weiqi::KataGoGtp gtp;
    const auto status = gtp.status();
    assert(status == "KataGo GTP 已就绪");
    assert(gtp.new_game(19));
    assert(gtp.play(weiqi::Stone::black, 3, 3, 19));
    const auto move = gtp.genmove(weiqi::Stone::white, 19);
    assert(move && move->first == 3 && move->second == 15);
}
