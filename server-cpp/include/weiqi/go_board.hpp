#pragma once

#include <optional>
#include <string>
#include <vector>

namespace weiqi {

enum class Stone : unsigned char { empty = 0, black = 1, white = 2 };

struct MoveResult {
    bool accepted{};
    std::string error;
    int captured{};
};

struct AreaScore { int black{}; int white{}; };

// Authoritative, dependency-free rule engine. Network transports may only call
// play/pass and must never mutate the board directly.
class GoBoard {
public:
    explicit GoBoard(int size = 19);

    [[nodiscard]] int size() const noexcept { return size_; }
    [[nodiscard]] Stone at(int x, int y) const;
    [[nodiscard]] Stone current_player() const noexcept { return current_player_; }
    [[nodiscard]] std::string signature() const;

    MoveResult play(int x, int y);
    void pass();
    [[nodiscard]] AreaScore chinese_area_score() const;

private:
    int size_;
    std::vector<Stone> points_;
    Stone current_player_{Stone::black};
    std::optional<std::string> position_before_last_move_;

    [[nodiscard]] bool in_bounds(int x, int y) const noexcept;
    [[nodiscard]] int index(int x, int y) const noexcept;
    [[nodiscard]] std::vector<int> group_at(int start) const;
    [[nodiscard]] bool has_liberty(const std::vector<int>& group) const;
    int remove_group(const std::vector<int>& group);
    [[nodiscard]] static Stone other(Stone stone) noexcept;
};

} // namespace weiqi
