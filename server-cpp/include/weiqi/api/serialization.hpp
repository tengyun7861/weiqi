#pragma once
#include "weiqi/game_service.hpp"
#include "weiqi/replay.hpp"
#include <nlohmann/json.hpp>

namespace weiqi::api {
nlohmann::json snapshot_json(const GameSnapshot &state);
nlohmann::json event_json(const RealtimeEvent &event);
nlohmann::json replay_json(const Replay &replay);
} // namespace weiqi::api
