#include "weiqi/auth.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace weiqi {
namespace {
constexpr ULONG kIterations = 600000;

std::string hex(const std::vector<unsigned char>& bytes) {
    std::ostringstream out;
    for (const auto byte : bytes) out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    return out.str();
}

std::vector<unsigned char> random_bytes(size_t size) {
    std::vector<unsigned char> result(size);
    if (BCryptGenRandom(nullptr, result.data(), static_cast<ULONG>(result.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
        throw std::runtime_error("无法生成安全随机数");
    }
    return result;
}

std::vector<unsigned char> sha256(const std::string& text) {
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    DWORD object_length{}, bytes{};
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0 ||
        BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&object_length), sizeof(object_length), &bytes, 0) < 0) {
        if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
        throw std::runtime_error("无法初始化 SHA-256");
    }
    std::vector<unsigned char> object(object_length), output(32);
    const NTSTATUS status = BCryptCreateHash(algorithm, &hash, object.data(), object_length, nullptr, 0, 0);
    if (status >= 0) BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(text.data())), static_cast<ULONG>(text.size()), 0);
    if (status >= 0) BCryptFinishHash(hash, output.data(), static_cast<ULONG>(output.size()), 0);
    if (hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status < 0) throw std::runtime_error("无法计算 SHA-256");
    return output;
}

std::string password_hash(const std::string& password, const std::vector<unsigned char>& salt) {
    BCRYPT_ALG_HANDLE algorithm{};
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0) {
        throw std::runtime_error("无法初始化密码哈希");
    }
    std::vector<unsigned char> output(32);
    const NTSTATUS status = BCryptDeriveKeyPBKDF2(algorithm,
        reinterpret_cast<PUCHAR>(const_cast<char*>(password.data())), static_cast<ULONG>(password.size()),
        const_cast<PUCHAR>(salt.data()), static_cast<ULONG>(salt.size()), kIterations,
        output.data(), static_cast<ULONG>(output.size()), 0);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status < 0) throw std::runtime_error("无法生成密码哈希");
    return "pbkdf2-sha256$600000$" + hex(salt) + '$' + hex(output);
}

bool constant_time_equal(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) return false;
    unsigned char diff{};
    for (size_t i = 0; i < left.size(); ++i) diff |= static_cast<unsigned char>(left[i] ^ right[i]);
    return diff == 0;
}
} // namespace

void AuthService::register_user(const std::string& nickname, const std::string& password) {
    if (nickname.size() < 2 || nickname.size() > 24 || password.size() < 8 || password.size() > 128) {
        throw std::invalid_argument("昵称须为 2–24 个字符，密码至少 8 个字符");
    }
    const auto salt = random_bytes(16);
    const auto encoded = password_hash(password, salt);
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt* statement{};
    sqlite3_prepare_v2(database_.handle(), "INSERT INTO users(nickname,password_hash) VALUES(?,?)", -1, &statement, nullptr);
    sqlite3_bind_text(statement, 1, nickname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, encoded.c_str(), -1, SQLITE_TRANSIENT);
    const int result = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE) throw std::invalid_argument("昵称已被占用");
}

std::string AuthService::login(const std::string& nickname, const std::string& password) {
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt* statement{};
    sqlite3_prepare_v2(database_.handle(), "SELECT id,password_hash FROM users WHERE nickname=?", -1, &statement, nullptr);
    sqlite3_bind_text(statement, 1, nickname.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(statement) != SQLITE_ROW) { sqlite3_finalize(statement); throw std::invalid_argument("昵称或密码错误"); }
    const int user_id = sqlite3_column_int(statement, 0);
    const std::string stored(reinterpret_cast<const char*>(sqlite3_column_text(statement, 1)));
    sqlite3_finalize(statement);
    const auto first = stored.find('$'), second = stored.find('$', first + 1), third = stored.find('$', second + 1);
    if (first == std::string::npos || second == std::string::npos || third == std::string::npos) throw std::runtime_error("密码记录格式无效");
    std::vector<unsigned char> salt;
    const std::string salt_hex = stored.substr(second + 1, third - second - 1);
    for (size_t i = 0; i < salt_hex.size(); i += 2) salt.push_back(static_cast<unsigned char>(std::stoul(salt_hex.substr(i, 2), nullptr, 16)));
    if (!constant_time_equal(stored, password_hash(password, salt))) throw std::invalid_argument("昵称或密码错误");
    const std::string token = hex(random_bytes(32));
    const auto expires = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 24 * 60 * 60;
    sqlite3_prepare_v2(database_.handle(), "INSERT INTO sessions(token_hash,user_id,expires_at) VALUES(?,?,?)", -1, &statement, nullptr);
    const auto token_hash = hex(sha256(token));
    sqlite3_bind_text(statement, 1, token_hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(statement, 2, user_id);
    sqlite3_bind_int64(statement, 3, expires);
    const int result = sqlite3_step(statement); sqlite3_finalize(statement);
    if (result != SQLITE_DONE) throw std::runtime_error("无法创建登录会话");
    return token;
}

std::optional<AuthenticatedUser> AuthService::authenticate(const std::string& bearer_token) {
    if (bearer_token.empty()) return std::nullopt;
    std::lock_guard lock(database_.mutex());
    sqlite3_stmt* statement{};
    sqlite3_prepare_v2(database_.handle(), "SELECT u.id,u.nickname FROM sessions s JOIN users u ON u.id=s.user_id WHERE s.token_hash=? AND s.expires_at>?", -1, &statement, nullptr);
    const auto token_hash = hex(sha256(bearer_token));
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    sqlite3_bind_text(statement, 1, token_hash.c_str(), -1, SQLITE_TRANSIENT); sqlite3_bind_int64(statement, 2, now);
    std::optional<AuthenticatedUser> user;
    if (sqlite3_step(statement) == SQLITE_ROW) user = AuthenticatedUser{sqlite3_column_int(statement, 0), reinterpret_cast<const char*>(sqlite3_column_text(statement, 1))};
    sqlite3_finalize(statement); return user;
}

} // namespace weiqi
