#include "weiqi/api_server.hpp"

#include <crow.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace weiqi {
namespace {
using json = nlohmann::json;
struct ClientSession { int user_id{}; std::string nickname; std::string room_code; bool spectating{}; int last_request_sequence{}; };

crow::response reply(bool ok, json data = json::object(), std::string error = {}, int status = 200) { json body{{"ok", ok}, {"data", std::move(data)}, {"error", error.empty() ? json(nullptr) : json(error)}}; crow::response result(status, body.dump()); result.set_header("Content-Type", "application/json; charset=utf-8"); return result; }
std::string bearer(const crow::request& request) { const auto header = request.get_header_value("Authorization"); constexpr std::string_view prefix = "Bearer "; return header.starts_with(prefix) ? header.substr(prefix.size()) : std::string{}; }
std::optional<AuthenticatedUser> require_user(const crow::request& request, AuthService& auth) { return auth.authenticate(bearer(request)); }
json snapshot_json(const GameSnapshot& snapshot) { return {{"roomCode", snapshot.room_code}, {"boardSize", snapshot.board_size}, {"currentPlayer", snapshot.current_player == Stone::black ? "black" : "white"}, {"stones", snapshot.stones}, {"blackTime", snapshot.black_time}, {"whiteTime", snapshot.white_time}, {"blackByoRemaining", snapshot.black_byo_remaining}, {"whiteByoRemaining", snapshot.white_byo_remaining}, {"byoYomiSeconds", snapshot.byo_yomi_seconds}, {"consecutivePasses", snapshot.consecutive_passes}, {"finished", snapshot.finished}, {"sequence", snapshot.sequence}, {"blackUserId", snapshot.black_user}, {"whiteUserId", snapshot.white_user}, {"aiWhite", snapshot.ai_white}}; }
json event_json(const RealtimeEvent& event) { json payload = event.snapshot.room_code.empty() ? json{{"roomCode", event.room_code}} : snapshot_json(event.snapshot); if (!event.message.empty()) payload["message"] = event.message; if (!event.nickname.empty()) payload["nickname"] = event.nickname; return {{"type", event.type}, {"payload", payload}, {"sequence", event.snapshot.sequence}}; }
}

int run_api_server(AuthService& auth, GameService& games, KataGoGtp& katago, unsigned short port) {
    crow::SimpleApp app;
    std::mutex clients_mutex; std::unordered_map<crow::websocket::connection*, ClientSession> clients;
    auto send_error = [](crow::websocket::connection& connection, const std::string& message, const std::string& request_id = {}) { connection.send_text(json{{"type", "error"}, {"requestId", request_id}, {"payload", {{"message", message}}}, {"sequence", 0}}.dump()); };
    auto rebuild_ai_board = [&](const std::string& room_code, int board_size) {
        if (!katago.new_game(board_size)) throw std::runtime_error(katago.status());
        for (const auto& move : games.moves_for_room(room_code)) if (!katago.play(move.color, move.x, move.y, board_size)) throw std::runtime_error(katago.status());
    };
    games.set_event_listener([&](const RealtimeEvent& event) {
        const auto wire = event_json(event).dump(); std::lock_guard lock(clients_mutex);
        for (auto& [connection, session] : clients) {
            const bool directed = event.user_id && *event.user_id == session.user_id;
            const bool in_room = !event.room_code.empty() && event.room_code == session.room_code;
            if (!directed && !in_room) continue;
            if (event.type == "match.found" || event.type == "room.snapshot") session.room_code = event.room_code;
            connection->send_text(wire);
        }
    });

    CROW_ROUTE(app, "/api/v1/health")([] { return reply(true, {{"status", "ok"}}); });
    CROW_ROUTE(app, "/api/v1/katago")([&] { const auto status = katago.status(); return reply(true, {{"available", katago.available() && status == "KataGo GTP 已就绪"}, {"reason", status}}); });
    CROW_ROUTE(app, "/api/v1/auth/register").methods(crow::HTTPMethod::Post)([&](const crow::request& request) { try { const auto body = json::parse(request.body); auth.register_user(body.at("nickname"), body.at("password")); return reply(true, {{"message", "注册成功，请登录"}}, {}, 201); } catch (const std::exception& error) { return reply(false, {}, error.what(), 400); } });
    CROW_ROUTE(app, "/api/v1/auth/login").methods(crow::HTTPMethod::Post)([&](const crow::request& request) { try { const auto body = json::parse(request.body); return reply(true, {{"token", auth.login(body.at("nickname"), body.at("password"))}, {"expiresIn", 86400}}); } catch (const std::exception& error) { return reply(false, {}, error.what(), 401); } });
    CROW_ROUTE(app, "/api/v1/rooms").methods(crow::HTTPMethod::Get)([&](const crow::request& request) { if (!require_user(request, auth)) return reply(false, {}, "登录已过期或无效", 401); json rooms = json::array(); for (const auto& room : games.public_rooms()) rooms.push_back({{"roomCode", room.code}, {"boardSize", room.board_size}, {"status", room.status}}); return reply(true, rooms); });
    CROW_ROUTE(app, "/api/v1/rooms").methods(crow::HTTPMethod::Post)([&](const crow::request& request) { const auto user = require_user(request, auth); if (!user) return reply(false, {}, "登录已过期或无效", 401); try { const auto body = json::parse(request.body); const auto code = games.create_room(user->id, body.value("boardSize", 19), body.value("isPublic", true), body.value("mainTimeSeconds", 600), body.value("byoYomiSeconds", 30)); return reply(true, {{"roomCode", code}}, {}, 201); } catch (const std::exception& error) { return reply(false, {}, error.what(), 400); } });
    CROW_ROUTE(app, "/api/v1/rooms/<string>/join").methods(crow::HTTPMethod::Post)([&](const crow::request& request, const std::string& code) { const auto user = require_user(request, auth); if (!user) return reply(false, {}, "登录已过期或无效", 401); try { return reply(true, snapshot_json(games.join_room(user->id, code, false))); } catch (const std::exception& error) { return reply(false, {}, error.what(), 400); } });
    CROW_ROUTE(app, "/api/v1/rooms/<string>/leave").methods(crow::HTTPMethod::Post)([&](const crow::request& request, const std::string& code) { const auto user = require_user(request, auth); if (!user) return reply(false, {}, "登录已过期或无效", 401); try { games.leave_room(user->id, code); return reply(true, {{"message", "已离开房间，座位会为重连保留"}}); } catch (const std::exception& error) { return reply(false, {}, error.what(), 400); } });
    CROW_ROUTE(app, "/api/v1/games").methods(crow::HTTPMethod::Get)([&](const crow::request& request) { const auto user = require_user(request, auth); if (!user) return reply(false, {}, "登录已过期或无效", 401); json records = json::array(); for (const auto& record : games.game_records(user->id)) records.push_back({{"id", record.id}, {"result", record.result}, {"finishReason", record.finish_reason}, {"endedAt", record.ended_at}}); return reply(true, records); });
    CROW_ROUTE(app, "/api/v1/games/<int>/sgf").methods(crow::HTTPMethod::Get)([&](const crow::request& request, int id) { const auto user = require_user(request, auth); if (!user) return reply(false, {}, "登录已过期或无效", 401); try { return reply(true, {{"sgf", games.game_sgf(user->id, id)}}); } catch (const std::exception& error) { return reply(false, {}, error.what(), 404); } });
    CROW_ROUTE(app, "/api/v1/ai/games").methods(crow::HTTPMethod::Post)([&](const crow::request& request) { const auto user=require_user(request,auth); if(!user) return reply(false,{},"登录已过期或无效",401); const auto body=json::parse(request.body); const int size=body.value("boardSize",19); if(katago.status()!="KataGo GTP 已就绪" || !katago.new_game(size)) return reply(false,{},katago.status(),503); try { const auto code=games.create_ai_room(user->id,size,body.value("mainTimeSeconds",600),body.value("byoYomiSeconds",30)); return reply(true,{{"roomCode",code}}, {},201); } catch(const std::exception& error){ return reply(false,{},error.what(),400); } });

    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([](crow::websocket::connection& connection) { connection.send_text(json{{"type", "session.ready"}, {"payload", {{"message", "请先发送 auth 事件"}}}, {"sequence", 0}}.dump()); })
        .onclose([&](crow::websocket::connection& connection, const std::string&, uint16_t) { ClientSession closing; bool known{}; { std::lock_guard lock(clients_mutex); const auto it = clients.find(&connection); if (it != clients.end()) { closing = it->second; known = true; clients.erase(it); } } if (known && !closing.room_code.empty()) games.leave_room(closing.user_id, closing.room_code); })
        .onmessage([&](crow::websocket::connection& connection, const std::string& message, bool is_binary) {
            if (is_binary) { send_error(connection, "不支持二进制消息"); return; }
            try {
                const auto incoming = json::parse(message); const auto type = incoming.value("type", ""); const auto request_id = incoming.value("requestId", "");
                if (type == "auth") {
                    const auto token = incoming.at("payload").at("token").get<std::string>(); const auto user = auth.authenticate(token); if (!user) { send_error(connection, "登录已过期或无效", request_id); return; }
                    { std::lock_guard lock(clients_mutex); clients[&connection] = {user->id, user->nickname, {}, false}; }
                    connection.send_text(json{{"type", "session.ready"}, {"requestId", request_id}, {"payload", {{"userId", user->id}, {"nickname", user->nickname}}}, {"sequence", 1}}.dump()); return;
                }
                ClientSession session; { std::lock_guard lock(clients_mutex); const auto it = clients.find(&connection); if (it == clients.end()) { send_error(connection, "请先使用 auth 鉴权", request_id); return; } const int incoming_sequence=incoming.value("sequence",0); if(incoming_sequence <= it->second.last_request_sequence) { send_error(connection,"实时请求序号无效或重复",request_id); return; } it->second.last_request_sequence=incoming_sequence; session = it->second; }
                const auto payload = incoming.value("payload", json::object());
                if (type == "match.join") { const auto matched = games.join_matchmaking(session.user_id, payload.value("boardSize", 19)); if (!matched) connection.send_text(json{{"type", "match.waiting"}, {"requestId", request_id}, {"payload", {{"message", "正在等待匹配对手"}}}, {"sequence", 0}}.dump()); }
                else if (type == "match.cancel") games.cancel_matchmaking(session.user_id);
                else if (type == "room.join" || type == "spectate.join") { const auto code = payload.at("roomCode").get<std::string>(); const auto state = games.join_room(session.user_id, code, type == "spectate.join"); { std::lock_guard lock(clients_mutex); clients[&connection].room_code = code; clients[&connection].spectating = type == "spectate.join"; } connection.send_text(json{{"type", "room.snapshot"}, {"requestId", request_id}, {"payload", snapshot_json(state)}, {"sequence", state.sequence}}.dump()); for(const auto& chat:games.chat_history(session.user_id,code)) connection.send_text(json{{"type","chat.message"},{"payload",{{"nickname",chat.nickname},{"message",chat.content},{"createdAt",chat.created_at}}},{"sequence",state.sequence}}.dump()); }
                else if (type == "room.leave") { if (!session.room_code.empty()) games.leave_room(session.user_id, session.room_code); std::lock_guard lock(clients_mutex); clients[&connection].room_code.clear(); }
                else if (type == "game.move") { const int x=payload.at("x"), y=payload.at("y"); const auto state=games.move(session.user_id, session.room_code, x, y); if(games.ai_turn(session.room_code)) { rebuild_ai_board(session.room_code,state.board_size); const auto answer=katago.genmove(Stone::white,state.board_size); if(answer) (void)games.ai_move(session.room_code,answer->first,answer->second); else (void)games.ai_pass(session.room_code); } }
                else if (type == "game.pass") { const auto state=games.pass(session.user_id, session.room_code); if(games.ai_turn(session.room_code)) { rebuild_ai_board(session.room_code,state.board_size); const auto answer=katago.genmove(Stone::white,state.board_size); if(answer) (void)games.ai_move(session.room_code,answer->first,answer->second); else (void)games.ai_pass(session.room_code); } }
                else if (type == "game.resign") (void)games.resign(session.user_id, session.room_code);
                else if (type == "chat.send") games.send_chat(session.user_id, session.nickname, session.room_code, payload.at("content"));
                else send_error(connection, "未知实时事件：" + type, request_id);
            } catch (const std::exception& error) { send_error(connection, error.what()); }
        });
    std::atomic_bool running{true}; std::jthread clock([&](std::stop_token stop) { while (!stop.stop_requested() && running) { std::this_thread::sleep_for(std::chrono::seconds(1)); games.tick(); } });
    app.port(port).multithreaded().run(); running = false; return 0;
}

} // namespace weiqi
