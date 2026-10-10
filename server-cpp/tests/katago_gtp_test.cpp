#include "weiqi/katago_gtp.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <cstdlib>

int main(int argc, char *argv[]) {
    assert(argc == 2);
    _putenv_s("WEIQI_KATAGO_COMMAND", argv[1]);
    weiqi::KataGoGtp gtp;
    const auto status = gtp.status();
    assert(status == "KataGo GTP 已就绪");
    assert(gtp.new_game(19));
    assert(gtp.play(weiqi::Stone::black, 3, 3, 19));
    const auto move = gtp.genmove(weiqi::Stone::white, 19);
    assert(move && move->first == 3 && move->second == 15);
    for (const auto &[text, kind] :
         {std::pair{"pass", weiqi::GtpMoveKind::pass}, std::pair{"resign", weiqi::GtpMoveKind::resign},
          std::pair{"I4", weiqi::GtpMoveKind::error}, std::pair{"D99", weiqi::GtpMoveKind::error},
          std::pair{"D4junk", weiqi::GtpMoveKind::error}, std::pair{"?", weiqi::GtpMoveKind::error}}) {
        _putenv_s("WEIQI_GTP_TEST_MOVE", text);
        weiqi::KataGoGtp fixture;
        assert(fixture.generate_move(weiqi::Stone::white, 9).kind == kind);
    }
    _putenv_s("WEIQI_GTP_TEST_MOVE", "D4");
    _putenv_s("WEIQI_GTP_TEST_DELAY_MS", "2000");
    _putenv_s("WEIQI_KATAGO_TIMEOUT_MS", "100");
    const auto started = std::chrono::steady_clock::now();
    {
        weiqi::KataGoGtp stalled;
        const auto answer = stalled.generate_move(weiqi::Stone::white, 9);
        assert(answer.kind == weiqi::GtpMoveKind::error && answer.error.find("超时") != std::string::npos);
    }
    assert(std::chrono::steady_clock::now() - started < std::chrono::seconds(1));
    _putenv_s("WEIQI_KATAGO_TIMEOUT_MS", "");
    _putenv_s("WEIQI_GTP_TEST_DELAY_MS", "");
    _putenv_s("WEIQI_GTP_TEST_MOVE", "");
}
