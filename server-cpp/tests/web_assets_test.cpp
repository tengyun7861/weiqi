#include "weiqi/web_assets.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <fstream>

int main() {
    const auto root = std::filesystem::temp_directory_path() / "weiqi_asset_test";
    std::filesystem::create_directories(root / "assets");
    std::ofstream(root / "index.html") << "<!doctype html>";
    std::ofstream(root / "assets/app-123.js") << "console.log('test')";
    std::ofstream(root / "secret.sqlite3") << "private";
    weiqi::WebAssets assets(root);
    assert(assets.resolve(""));
    assert(assets.resolve("assets/app-123.js")->immutable);
    assert(!assets.resolve("../outside.html"));
    assert(!assets.resolve("assets/../../outside.html"));
    assert(!assets.resolve("C:/Windows/system.ini"));
    assert(!assets.resolve("assets\\app-123.js"));
    assert(!assets.resolve("secret.sqlite3"));
    assert(!assets.resolve("missing.html"));
    std::filesystem::remove(root / "index.html");
    std::filesystem::remove(root / "assets/app-123.js");
    std::filesystem::remove(root / "secret.sqlite3");
    std::filesystem::remove(root / "assets");
    std::filesystem::remove(root);
}
