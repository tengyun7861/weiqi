#pragma once
#include "weiqi/auth.hpp"
#include <crow.h>
#include <nlohmann/json.hpp>

namespace weiqi::api {
using json = nlohmann::json;
crow::response reply(bool ok, json data = json::object(), std::string error = {}, int status = 200);
std::string bearer(const crow::request &request);
std::optional<AuthenticatedUser> user_for(const crow::request &request, AuthService &auth);
crow::response unauthorized();
} // namespace weiqi::api
