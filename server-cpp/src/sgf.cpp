#include "weiqi/sgf.hpp"

#include <cctype>
#include <cmath>
#include <map>
#include <stdexcept>

namespace weiqi {

std::string make_sgf(int board_size, double komi, const std::vector<SgfMove> &moves,
                     const std::string &result) {
    std::string sgf = "(;GM[1]FF[4]CA[UTF-8]RU[Chinese]SZ[" + std::to_string(board_size) + "]KM[" +
                      std::to_string(komi) + "]";
    if (!result.empty())
        sgf += "RE[" + result + "]";
    for (const auto &move : moves) {
        sgf += ';';
        sgf += move.color == Stone::black ? "B[" : "W[";
        if (move.x >= 0 && move.y >= 0) {
            sgf.push_back(static_cast<char>('a' + move.x));
            sgf.push_back(static_cast<char>('a' + move.y));
        }
        sgf += ']';
    }
    return sgf + ')';
}

SgfDocument parse_sgf_document(const std::string &sgf) {
    if (sgf.empty() || sgf.size() > 2 * 1024 * 1024)
        throw std::invalid_argument("SGF 内容为空或过大");
    size_t cursor{};
    auto whitespace = [&] {
        while (cursor < sgf.size() && std::isspace(static_cast<unsigned char>(sgf[cursor])))
            ++cursor;
    };
    auto value = [&]() {
        if (cursor >= sgf.size() || sgf[cursor++] != '[')
            throw std::invalid_argument("SGF 属性缺少值");
        std::string result;
        bool closed{};
        while (cursor < sgf.size()) {
            const char ch = sgf[cursor++];
            if (ch == ']') {
                closed = true;
                break;
            }
            if (ch == '\\') {
                if (cursor >= sgf.size())
                    throw std::invalid_argument("SGF 转义不完整");
                const char escaped = sgf[cursor++];
                if (escaped == '\r' || escaped == '\n') {
                    if (escaped == '\r' && cursor < sgf.size() && sgf[cursor] == '\n')
                        ++cursor;
                    continue;
                }
                result += escaped;
            } else
                result += ch;
        }
        if (!closed)
            throw std::invalid_argument("SGF 属性值未闭合");
        return result;
    };
    whitespace();
    if (cursor >= sgf.size() || sgf[cursor++] != '(')
        throw std::invalid_argument("无效的 SGF 文件");
    SgfDocument document;
    bool root = true;
    bool ended{};
    while (cursor < sgf.size()) {
        whitespace();
        if (cursor >= sgf.size())
            break;
        if (sgf[cursor] == '(')
            throw std::invalid_argument("当前导入仅支持无分支的主线棋谱");
        if (sgf[cursor] == ')') {
            ++cursor;
            ended = true;
            break;
        }
        if (sgf[cursor++] != ';')
            throw std::invalid_argument("SGF 节点格式无效");
        std::map<std::string, std::vector<std::string>> properties;
        while (cursor < sgf.size()) {
            whitespace();
            if (cursor >= sgf.size() || sgf[cursor] == ';' || sgf[cursor] == ')' || sgf[cursor] == '(')
                break;
            std::string key;
            while (cursor < sgf.size() && sgf[cursor] >= 'A' && sgf[cursor] <= 'Z')
                key += sgf[cursor++];
            if (key.empty() || properties.contains(key))
                throw std::invalid_argument("SGF 属性格式无效");
            whitespace();
            auto &values = properties[key];
            do {
                values.push_back(value());
                whitespace();
            } while (cursor < sgf.size() && sgf[cursor] == '[');
        }
        auto single = [&](const std::string &key) -> std::string {
            const auto it = properties.find(key);
            if (it == properties.end())
                return {};
            if (it->second.size() != 1)
                throw std::invalid_argument("SGF 单值属性格式无效");
            return it->second.front();
        };
        if (root) {
            const auto size = single("SZ");
            if (size != "9" && size != "13" && size != "19")
                throw std::invalid_argument("SGF 棋盘大小只能是 9、13 或 19");
            document.board_size = std::stoi(size);
            if (properties.contains("GM") && single("GM") != "1")
                throw std::invalid_argument("这不是围棋棋谱");
            if (properties.contains("KM")) {
                size_t used{};
                const auto komi = single("KM");
                document.komi = std::stod(komi, &used);
                if (used != komi.size() || !std::isfinite(document.komi))
                    throw std::invalid_argument("SGF 贴目无效");
            }
            document.result = single("RE");
            root = false;
        }
        if (properties.contains("AB") || properties.contains("AW") || properties.contains("AE") ||
            (properties.contains("HA") && single("HA") != "0"))
            throw std::invalid_argument("当前导入暂不支持让子或摆子棋谱");
        if (properties.contains("B") && properties.contains("W"))
            throw std::invalid_argument("SGF 节点包含双方着法");
        for (const auto &key : {"B", "W"})
            if (properties.contains(key)) {
                const auto point = single(key);
                SgfMove move{std::string(key) == "B" ? Stone::black : Stone::white, -1, -1};
                if (!point.empty()) {
                    if (point.size() != 2 || point[0] < 'a' || point[1] < 'a' ||
                        point[0] >= 'a' + document.board_size || point[1] >= 'a' + document.board_size)
                        throw std::invalid_argument("SGF 坐标无效");
                    move.x = point[0] - 'a';
                    move.y = point[1] - 'a';
                }
                document.moves.push_back(move);
            }
    }
    whitespace();
    if (root || !ended || cursor != sgf.size())
        throw std::invalid_argument("SGF 棋谱未闭合或包含多局");
    return document;
}

std::vector<SgfMove> parse_sgf_moves(const std::string &sgf, int expected_board_size) {
    auto document = parse_sgf_document(sgf);
    if (document.board_size != expected_board_size)
        throw std::invalid_argument("SGF 棋盘大小不匹配");
    return std::move(document.moves);
}

} // namespace weiqi
