#include "weiqi/go_board.hpp"

#include <array>
#include <queue>
#include <stdexcept>

namespace weiqi {

GoBoard::GoBoard(int size) : size_(size), points_(static_cast<size_t>(size) * size, Stone::empty) {
    if (size != 9 && size != 13 && size != 19) {
        throw std::invalid_argument("棋盘大小只能是 9、13 或 19");
    }
}

Stone GoBoard::at(int x, int y) const {
    if (!in_bounds(x, y)) {
        throw std::out_of_range("坐标超出棋盘范围");
    }
    return points_[index(x, y)];
}

std::string GoBoard::signature() const {
    std::string result = std::to_string(size_) + ':';
    result.reserve(result.size() + points_.size());
    for (const Stone point : points_) {
        result.push_back(static_cast<char>('0' + static_cast<unsigned char>(point)));
    }
    return result;
}

MoveResult GoBoard::play(int x, int y) {
    if (!in_bounds(x, y)) {
        return {false, "落子坐标超出棋盘范围", 0};
    }
    const int placed = index(x, y);
    if (points_[placed] != Stone::empty) {
        return {false, "该位置已有棋子", 0};
    }

    const auto before = signature();
    const auto original = points_;
    points_[placed] = current_player_;

    int captured = 0;
    const Stone opponent = other(current_player_);
    constexpr std::array<std::pair<int, int>, 4> directions{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
    for (const auto [dx, dy] : directions) {
        const int nx = x + dx;
        const int ny = y + dy;
        if (!in_bounds(nx, ny) || points_[index(nx, ny)] != opponent) {
            continue;
        }
        const auto group = group_at(index(nx, ny));
        if (!has_liberty(group)) {
            captured += remove_group(group);
        }
    }

    const auto own_group = group_at(placed);
    if (!has_liberty(own_group)) {
        points_ = original;
        return {false, "禁入点：该手棋没有气", 0};
    }
    if (position_before_last_move_ && signature() == *position_before_last_move_) {
        points_ = original;
        return {false, "打劫规则：不能立即回提", 0};
    }

    position_before_last_move_ = before;
    current_player_ = other(current_player_);
    return {true, {}, captured};
}

void GoBoard::pass() {
    position_before_last_move_.reset();
    current_player_ = other(current_player_);
}

AreaScore GoBoard::chinese_area_score() const {
    AreaScore score;
    std::vector<bool> visited(points_.size());
    constexpr std::array<std::pair<int, int>, 4> directions{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
    for (int start = 0; start < static_cast<int>(points_.size()); ++start) {
        if (visited[start]) continue;
        if (points_[start] == Stone::black) { ++score.black; visited[start] = true; continue; }
        if (points_[start] == Stone::white) { ++score.white; visited[start] = true; continue; }
        std::queue<int> pending; std::vector<int> territory; bool adjacent_black{}; bool adjacent_white{};
        pending.push(start); visited[start] = true;
        while (!pending.empty()) {
            const int point = pending.front(); pending.pop(); territory.push_back(point);
            const int x = point % size_, y = point / size_;
            for (const auto [dx, dy] : directions) {
                const int nx = x + dx, ny = y + dy;
                if (!in_bounds(nx, ny)) continue;
                const int adjacent = index(nx, ny);
                if (points_[adjacent] == Stone::empty && !visited[adjacent]) { visited[adjacent] = true; pending.push(adjacent); }
                else if (points_[adjacent] == Stone::black) adjacent_black = true;
                else if (points_[adjacent] == Stone::white) adjacent_white = true;
            }
        }
        if (adjacent_black && !adjacent_white) score.black += static_cast<int>(territory.size());
        if (adjacent_white && !adjacent_black) score.white += static_cast<int>(territory.size());
    }
    return score;
}

bool GoBoard::in_bounds(int x, int y) const noexcept {
    return x >= 0 && y >= 0 && x < size_ && y < size_;
}

int GoBoard::index(int x, int y) const noexcept { return y * size_ + x; }

std::vector<int> GoBoard::group_at(int start) const {
    const Stone color = points_[start];
    std::vector<bool> seen(points_.size());
    std::queue<int> pending;
    std::vector<int> group;
    pending.push(start);
    seen[start] = true;
    constexpr std::array<std::pair<int, int>, 4> directions{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};

    while (!pending.empty()) {
        const int point = pending.front();
        pending.pop();
        group.push_back(point);
        const int x = point % size_;
        const int y = point / size_;
        for (const auto [dx, dy] : directions) {
            const int nx = x + dx;
            const int ny = y + dy;
            if (!in_bounds(nx, ny)) continue;
            const int adjacent = index(nx, ny);
            if (!seen[adjacent] && points_[adjacent] == color) {
                seen[adjacent] = true;
                pending.push(adjacent);
            }
        }
    }
    return group;
}

bool GoBoard::has_liberty(const std::vector<int>& group) const {
    constexpr std::array<std::pair<int, int>, 4> directions{{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
    for (const int point : group) {
        const int x = point % size_;
        const int y = point / size_;
        for (const auto [dx, dy] : directions) {
            const int nx = x + dx;
            const int ny = y + dy;
            if (in_bounds(nx, ny) && points_[index(nx, ny)] == Stone::empty) return true;
        }
    }
    return false;
}

int GoBoard::remove_group(const std::vector<int>& group) {
    for (const int point : group) points_[point] = Stone::empty;
    return static_cast<int>(group.size());
}

Stone GoBoard::other(Stone stone) noexcept {
    return stone == Stone::black ? Stone::white : Stone::black;
}

} // namespace weiqi
