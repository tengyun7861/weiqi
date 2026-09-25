#pragma once

#include "weiqi/database.hpp"

#include <optional>
#include <string>

namespace weiqi {

struct AuthenticatedUser { int id{}; std::string nickname; };

class AuthService final {
public:
    explicit AuthService(Database& database) : database_(database) {}
    void register_user(const std::string& nickname, const std::string& password);
    [[nodiscard]] std::string login(const std::string& nickname, const std::string& password);
    [[nodiscard]] std::optional<AuthenticatedUser> authenticate(const std::string& bearer_token);

private:
    Database& database_;
};

} // namespace weiqi
