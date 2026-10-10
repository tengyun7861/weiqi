#include "weiqi/detail/database_helpers.hpp"
#include "weiqi/game_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace weiqi {
using detail::bind_text;
using detail::user_name;
void GameService::send_chat(int user_id, const std::string &nickname, const std::string &room_code,
                            const std::string &content) {
    if (content.empty() || content.size() > 500)
        throw std::invalid_argument("聊天内容须为 1–500 个字符");
    {
        std::lock_guard lock(mutex_);
        require_game(room_code);
    }
    {
        std::lock_guard lock(database_.mutex());
        sqlite3_stmt *statement{};
        sqlite3_prepare_v2(
            database_.handle(),
            "INSERT INTO chat_messages(room_id,user_id,content) SELECT id,?,? FROM rooms WHERE room_code=?",
            -1, &statement, nullptr);
        sqlite3_bind_int(statement, 1, user_id);
        bind_text(statement, 2, content);
        bind_text(statement, 3, room_code);
        if (sqlite3_step(statement) != SQLITE_DONE) {
            sqlite3_finalize(statement);
            throw std::runtime_error("无法保存聊天记录");
        }
        sqlite3_finalize(statement);
    }
    emit({"chat.message", room_code, std::nullopt, {}, content, nickname});
}

std::vector<ChatMessage> GameService::chat_history(int user_id, const std::string &room_code) const {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt *statement{};
    sqlite3_prepare_v2(
        database_.handle(),
        "SELECT u.nickname,c.content,c.created_at FROM chat_messages c JOIN rooms r ON r.id=c.room_id JOIN "
        "users u ON u.id=c.user_id WHERE r.room_code=? ORDER BY c.id DESC LIMIT 100",
        -1, &statement, nullptr);
    bind_text(statement, 1, room_code);
    (void)user_id;
    std::vector<ChatMessage> messages;
    while (sqlite3_step(statement) == SQLITE_ROW) {
        const auto nickname = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
        const auto content = reinterpret_cast<const char *>(sqlite3_column_text(statement, 1));
        const auto created = reinterpret_cast<const char *>(sqlite3_column_text(statement, 2));
        messages.push_back({nickname ? nickname : "", content ? content : "", created ? created : ""});
    }
    sqlite3_finalize(statement);
    std::reverse(messages.begin(), messages.end());
    return messages;
}

} // namespace weiqi
