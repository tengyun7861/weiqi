#include "weiqi/api/serialization.hpp"

namespace weiqi::api {
using json = nlohmann::json;
json snapshot_json(const GameSnapshot &state) {
    return {{"roomCode", state.room_code},
            {"boardSize", state.board_size},
            {"currentPlayer", state.current_player == Stone::black ? "black" : "white"},
            {"stones", state.stones},
            {"blackTime", state.black_time},
            {"whiteTime", state.white_time},
            {"blackByoRemaining", state.black_byo_remaining},
            {"whiteByoRemaining", state.white_byo_remaining},
            {"byoYomiSeconds", state.byo_yomi_seconds},
            {"consecutivePasses", state.consecutive_passes},
            {"finished", state.finished},
            {"sequence", state.sequence},
            {"blackUserId", state.black_user},
            {"whiteUserId", state.white_user},
            {"aiWhite", state.ai_white},
            {"blackName", state.black_name},
            {"whiteName", state.white_name},
            {"result", state.result},
            {"finishReason", state.finish_reason},
            {"moveCount", state.move_count},
            {"lastMove", state.last_x < 0 ? json(nullptr) : json{{"x", state.last_x}, {"y", state.last_y}}}};
}
json event_json(const RealtimeEvent &event) {
    json payload = event.snapshot.room_code.empty() ? json{{"roomCode", event.room_code}}
                                                    : snapshot_json(event.snapshot);
    if (!event.message.empty())
        payload["message"] = event.message;
    if (!event.nickname.empty())
        payload["nickname"] = event.nickname;
    return {{"type", event.type}, {"payload", std::move(payload)}, {"sequence", event.snapshot.sequence}};
}
json replay_json(const Replay &replay) {
    json frames = json::array();
    for (const auto &frame : replay.frames)
        frames.push_back({{"stones", frame.stones},
                          {"currentPlayer", frame.current_player == Stone::black ? "black" : "white"},
                          {"lastMove", frame.last_x < 0 ? json(nullptr)
                                                        : json{{"x", frame.last_x}, {"y", frame.last_y}}}});
    json moves = json::array();
    for (const auto &move : replay.moves)
        moves.push_back(
            {{"color", move.color == Stone::black ? "black" : "white"}, {"x", move.x}, {"y", move.y}});
    return {{"boardSize", replay.board_size},
            {"komi", replay.komi},
            {"result", replay.result},
            {"moves", std::move(moves)},
            {"frames", std::move(frames)}};
}
} // namespace weiqi::api
