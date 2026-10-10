#include "weiqi/web_assets.hpp"

#include <map>
#include <system_error>

namespace weiqi {
WebAssets::WebAssets(std::filesystem::path root) {
    std::error_code error;
    root_ = std::filesystem::weakly_canonical(std::move(root), error);
    if (error)
        root_.clear();
}

std::optional<WebAsset> WebAssets::resolve(const std::string &relative_path) const {
    if (root_.empty() || relative_path.find('\0') != std::string::npos ||
        relative_path.find('\\') != std::string::npos || relative_path.find(':') != std::string::npos)
        return std::nullopt;
    const std::filesystem::path relative(relative_path.empty() ? "index.html" : relative_path);
    if (relative.is_absolute())
        return std::nullopt;
    for (const auto &part : relative)
        if (part == ".." || part == ".")
            return std::nullopt;
    std::error_code error;
    const auto candidate = std::filesystem::weakly_canonical(root_ / relative, error);
    if (error || !std::filesystem::is_regular_file(candidate, error))
        return std::nullopt;
    const auto inside = candidate.lexically_relative(root_);
    if (inside.empty() || inside.is_absolute())
        return std::nullopt;
    for (const auto &part : inside)
        if (part == "..")
            return std::nullopt;
    static const std::map<std::string, std::string> types{{".html", "text/html; charset=utf-8"},
                                                          {".js", "text/javascript; charset=utf-8"},
                                                          {".css", "text/css; charset=utf-8"},
                                                          {".svg", "image/svg+xml"},
                                                          {".png", "image/png"},
                                                          {".webp", "image/webp"},
                                                          {".ico", "image/x-icon"},
                                                          {".woff2", "font/woff2"}};
    const auto type = types.find(candidate.extension().string());
    if (type == types.end())
        return std::nullopt;
    return WebAsset{candidate, type->second, relative.begin()->string() == "assets"};
}
} // namespace weiqi
