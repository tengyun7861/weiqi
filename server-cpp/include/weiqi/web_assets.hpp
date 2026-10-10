#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace weiqi {
struct WebAsset {
    std::filesystem::path file;
    std::string content_type;
    bool immutable{};
};

// Only files inside the compiled browser assets directory can be served.
class WebAssets final {
  public:
    explicit WebAssets(std::filesystem::path root);
    [[nodiscard]] std::optional<WebAsset> resolve(const std::string &relative_path) const;

  private:
    std::filesystem::path root_;
};
} // namespace weiqi
