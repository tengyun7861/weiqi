#include "weiqi/katago_gtp.hpp"

#include <windows.h>

#include <array>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <vector>

namespace weiqi {

KataGoGtp::KataGoGtp() {
    if (const char* configured = std::getenv("WEIQI_KATAGO_COMMAND"); configured && *configured) command_line_ = configured;
    else reason_ = "未配置 KataGo：请设置 WEIQI_KATAGO_COMMAND（例如 katago.exe gtp -config ... -model ...）";
}

KataGoGtp::~KataGoGtp() { stop(); }
bool KataGoGtp::available() const noexcept { return !command_line_.empty(); }

bool KataGoGtp::start_locked() {
    if (process_) return true;
    if (command_line_.empty()) return false;
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE}; HANDLE child_stdin{}, child_stdout{}, parent_stdin{}, parent_stdout{};
    if (!CreatePipe(&parent_stdout, &child_stdout, &security, 0) || !SetHandleInformation(parent_stdout, HANDLE_FLAG_INHERIT, 0) ||
        !CreatePipe(&child_stdin, &parent_stdin, &security, 0) || !SetHandleInformation(parent_stdin, HANDLE_FLAG_INHERIT, 0)) {
        reason_ = "无法创建 KataGo GTP 管道"; if (child_stdout) CloseHandle(child_stdout); if (parent_stdout) CloseHandle(parent_stdout); if (child_stdin) CloseHandle(child_stdin); if (parent_stdin) CloseHandle(parent_stdin); return false;
    }
    STARTUPINFOA startup{}; startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE; startup.hStdInput = child_stdin; startup.hStdOutput = child_stdout; startup.hStdError = child_stdout;
    std::string executable;
    if (command_line_.front() == '"') { const auto close = command_line_.find('"', 1); executable = close == std::string::npos ? command_line_ : command_line_.substr(1, close - 1); }
    else executable = command_line_.substr(0, command_line_.find_first_of(" \t"));
    PROCESS_INFORMATION info{}; std::vector<char> mutable_command(command_line_.begin(), command_line_.end()); mutable_command.push_back('\0');
    const BOOL created = CreateProcessA(executable.c_str(), mutable_command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &info);
    CloseHandle(child_stdin); CloseHandle(child_stdout);
    if (!created) { CloseHandle(parent_stdin); CloseHandle(parent_stdout); reason_ = "无法启动 KataGo GTP 子进程"; return false; }
    CloseHandle(info.hThread); process_ = info.hProcess; stdin_write_ = parent_stdin; stdout_read_ = parent_stdout;
    if (!command_locked("protocol_version")) { stop(); return false; }
    reason_ = "KataGo GTP 已就绪"; return true;
}

std::optional<std::string> KataGoGtp::command_locked(const std::string& command) {
    if (!start_locked() && !process_) return std::nullopt;
    const std::string request = command + "\n"; DWORD written{};
    if (!WriteFile(static_cast<HANDLE>(stdin_write_), request.data(), static_cast<DWORD>(request.size()), &written, nullptr) || written != request.size()) { reason_ = "向 KataGo GTP 发送命令失败"; stop(); return std::nullopt; }
    std::string response; std::array<char, 256> buffer{};
    auto complete_response = [&response] { return response.find("\n\n") != std::string::npos || response.find("\r\n\r\n") != std::string::npos; };
    while (!complete_response() && response.size() < 65536) {
        DWORD read{}; if (!ReadFile(static_cast<HANDLE>(stdout_read_), buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr) || read == 0) { reason_ = "KataGo GTP 意外退出"; stop(); return std::nullopt; }
        response.append(buffer.data(), read);
    }
    if (!response.starts_with("=")) { reason_ = "KataGo GTP 返回错误：" + response; return std::nullopt; }
    auto text = response.substr(1); auto end = text.find("\r\n\r\n"); if (end == std::string::npos) end = text.find("\n\n"); if (end != std::string::npos) text.resize(end);
    while (!text.empty() && (text.front() == ' ' || text.front() == '\n' || text.front() == '\r')) text.erase(text.begin());
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.pop_back(); return text;
}

std::string KataGoGtp::status() { std::lock_guard lock(mutex_); if (command_line_.empty()) return reason_; (void)start_locked(); return reason_; }
bool KataGoGtp::new_game(int board_size) { std::lock_guard lock(mutex_); return command_locked("boardsize " + std::to_string(board_size)).has_value() && command_locked("clear_board").has_value(); }
std::string KataGoGtp::vertex(int x, int y, int board_size) { if (x < 0 || y < 0) return "pass"; char column = static_cast<char>('A' + x + (x >= 8)); return std::string(1, column) + std::to_string(board_size - y); }
bool KataGoGtp::play(Stone color, int x, int y, int board_size) { std::lock_guard lock(mutex_); return command_locked("play " + std::string(color == Stone::black ? "B " : "W ") + vertex(x, y, board_size)).has_value(); }
std::optional<std::pair<int, int>> KataGoGtp::genmove(Stone color, int board_size) {
    std::lock_guard lock(mutex_); const auto response = command_locked("genmove " + std::string(color == Stone::black ? "B" : "W")); if (!response || *response == "pass" || *response == "resign") return std::nullopt;
    const char column = static_cast<char>(std::toupper(static_cast<unsigned char>((*response)[0]))); int row{}; try { row = std::stoi(response->substr(1)); } catch (...) { reason_ = "KataGo 返回了无效坐标"; return std::nullopt; }
    const int x = column - 'A' - (column > 'I'); const int y = board_size - row; if (x < 0 || y < 0 || x >= board_size || y >= board_size) { reason_ = "KataGo 返回了越界坐标"; return std::nullopt; } return std::pair{x, y};
}

void KataGoGtp::stop() noexcept {
    if (stdin_write_) { CloseHandle(static_cast<HANDLE>(stdin_write_)); stdin_write_ = nullptr; }
    if (stdout_read_) { CloseHandle(static_cast<HANDLE>(stdout_read_)); stdout_read_ = nullptr; }
    if (process_) { const auto process = static_cast<HANDLE>(process_); if (WaitForSingleObject(process, 100) == WAIT_TIMEOUT) TerminateProcess(process, 0); CloseHandle(process); process_ = nullptr; }
}

} // namespace weiqi
