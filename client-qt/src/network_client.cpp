#include "network_client.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QUuid>
#include <QUrl>

NetworkClient::NetworkClient(QObject* parent) : QObject(parent) {
    server_url_=QSettings().value("network/serverUrl", server_url_).toString();
    reconnect_timer_.setSingleShot(true); reconnect_timer_.setInterval(2000);
    connect(&reconnect_timer_, &QTimer::timeout, this, [this] { if (loggedIn() && socket_.state() == QAbstractSocket::UnconnectedState) connectRealtime(); });
    connect(&socket_, &QTcpSocket::connected, this, [this] {
        const QUrl url(server_url_); QByteArray random(16, Qt::Uninitialized);
        for (auto& byte : random) byte = static_cast<char>(QRandomGenerator::global()->generate() & 0xff);
        const QByteArray request = "GET /ws HTTP/1.1\r\nHost: " + url.host().toUtf8() + ':' + QByteArray::number(url.port(8081)) + "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: " + random.toBase64() + "\r\nSec-WebSocket-Version: 13\r\n\r\n";
        socket_.write(request); setStatus("正在建立实时连接");
    });
    connect(&socket_, &QTcpSocket::readyRead, this, &NetworkClient::consumeSocketData);
    connect(&socket_, &QTcpSocket::disconnected, this, [this] { websocket_ready_ = false; realtime_authenticated_ = false; if (loggedIn()) { setStatus("实时连接已断开，正在自动重连"); if (!reconnect_timer_.isActive()) reconnect_timer_.start(); } });
    connect(&socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) { setStatus("实时连接失败：" + socket_.errorString()); });
}

void NetworkClient::setServerUrl(const QString& url) { const auto normalized = url.trimmed().remove(QRegularExpression("/$")); if (normalized.isEmpty() || normalized == server_url_) return; server_url_ = normalized; QSettings().setValue("network/serverUrl",server_url_); emit serverUrlChanged(); setStatus("服务端地址已更新，等待连接"); }
void NetworkClient::setStatus(const QString& value) { if (status_ != value) { status_ = value; emit statusChanged(); } }

void NetworkClient::postJson(const QString& path, const QJsonObject& body, std::function<void(const QJsonObject&)> success) {
    QNetworkRequest request{QUrl(server_url_ + path)}; request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    const auto reply = network_.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, success = std::move(success)] {
        const auto document = QJsonDocument::fromJson(reply->readAll()); const auto object = document.object(); const auto error = object.value("error"); reply->deleteLater();
        if (document.isNull() || !object.value("ok").toBool()) { const auto message = error.isString() ? error.toString() : "服务端返回无效响应"; setStatus(message); emit requestFailed(message); return; } success(object.value("data").toObject());
    });
}

void NetworkClient::registerAccount(const QString& nickname, const QString& password) { postJson("/api/v1/auth/register", {{"nickname", nickname}, {"password", password}}, [this](const QJsonObject&) { setStatus("注册成功，请登录"); }); }
void NetworkClient::login(const QString& nickname, const QString& password) { postJson("/api/v1/auth/login", {{"nickname", nickname}, {"password", password}}, [this, nickname](const QJsonObject& data) { token_ = data.value("token").toString(); nickname_ = nickname; emit sessionChanged(); setStatus("登录成功"); loadKataGoStatus(); connectRealtime(); }); }
void NetworkClient::logout() { reconnect_timer_.stop(); token_.clear(); nickname_.clear(); room_code_.clear(); spectating_=false; pending_room_.clear(); pending_match_board_=0; socket_.disconnectFromHost(); websocket_ready_=false; realtime_authenticated_=false; emit sessionChanged(); emit roomChanged(); setStatus("已退出登录"); }

void NetworkClient::loadKataGoStatus() {
    const auto reply = network_.get(QNetworkRequest{QUrl(server_url_ + "/api/v1/katago")});
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const auto document=QJsonDocument::fromJson(reply->readAll()); const auto object=document.object(); reply->deleteLater();
        const auto data=object.value("data").toObject(); ai_available_=object.value("ok").toBool() && data.value("available").toBool(); katago_reason_=data.value("reason").toString(ai_available_ ? "KataGo GTP 已就绪" : "无法获取 KataGo 状态"); emit katagoStatusChanged();
    });
}

void NetworkClient::loadPublicRooms() {
    QNetworkRequest request{QUrl(server_url_ + "/api/v1/rooms")}; request.setRawHeader("Authorization", "Bearer " + token_.toUtf8());
    const auto reply = network_.get(request); connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const auto document = QJsonDocument::fromJson(reply->readAll()); const auto object = document.object(); reply->deleteLater(); if (!object.value("ok").toBool()) { emit requestFailed(object.value("error").toString("无法加载公开房间")); return; }
        QVariantList rooms; for (const auto& item : object.value("data").toArray()) rooms.append(item.toObject().toVariantMap()); emit roomsReceived(rooms); setStatus("公开大厅已刷新");
    });
}

void NetworkClient::connectRealtime() {
    if (token_.isEmpty()) { emit requestFailed("请先登录"); return; }
    const QUrl url(server_url_); if (!url.isValid() || (url.scheme() != "http" && url.scheme() != "ws")) { emit requestFailed("实时连接仅支持 ws:// 或 http:// 局域网地址"); return; }
    reconnect_timer_.stop(); socket_buffer_.clear(); websocket_ready_ = false; realtime_authenticated_ = false; outgoing_sequence_ = 0; socket_.abort(); socket_.connectToHost(url.host(), static_cast<quint16>(url.port(8081)));
}

void NetworkClient::consumeSocketData() {
    socket_buffer_ += socket_.readAll();
    if (!websocket_ready_) {
        const auto header_end = socket_buffer_.indexOf("\r\n\r\n"); if (header_end < 0) return;
        const auto header = socket_buffer_.left(header_end); socket_buffer_.remove(0, header_end + 4);
        if (!header.startsWith("HTTP/1.1 101")) { setStatus("实时服务端拒绝 WebSocket 连接"); socket_.disconnectFromHost(); return; }
        websocket_ready_ = true; setStatus("实时连接已建立"); sendRealtimeJson({{"type", "auth"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{{"token", token_}}}, {"sequence", last_sequence_}});
    }
    while (socket_buffer_.size() >= 2) {
        const quint8 first = static_cast<quint8>(socket_buffer_[0]), second = static_cast<quint8>(socket_buffer_[1]); const bool masked = second & 0x80; quint64 length = second & 0x7f; int offset = 2;
        if (length == 126) { if (socket_buffer_.size() < 4) return; length = (static_cast<quint8>(socket_buffer_[2]) << 8) | static_cast<quint8>(socket_buffer_[3]); offset = 4; }
        else if (length == 127) { emit requestFailed("实时消息过长"); socket_.disconnectFromHost(); return; }
        if (masked) { emit requestFailed("服务端发送了不合规的掩码帧"); socket_.disconnectFromHost(); return; }
        if (socket_buffer_.size() < offset + static_cast<int>(length)) return;
        const auto opcode = first & 0x0f; const auto payload = socket_buffer_.mid(offset, static_cast<int>(length)); socket_buffer_.remove(0, offset + static_cast<int>(length));
        if (opcode == 0x8) { socket_.disconnectFromHost(); return; }
        if (opcode == 0x9) { socket_.write(QByteArray("\x8a\0", 2)); continue; }
        if (opcode != 0x1) continue;
        const auto document = QJsonDocument::fromJson(payload); if (!document.isObject()) continue; const auto object = document.object(); const int sequence = object.value("sequence").toInt(last_sequence_);
        if (sequence < last_sequence_) continue; last_sequence_ = sequence; const auto type = object.value("type").toString(); const auto event_payload = object.value("payload").toObject();
        if (type == "session.ready" && event_payload.contains("userId")) { realtime_authenticated_ = true; if (pending_match_board_ > 0) { const int board = pending_match_board_; pending_match_board_ = 0; joinMatch(board); } if (!pending_room_.isEmpty()) { const auto room = pending_room_; const bool spectate = pending_spectate_; pending_room_.clear(); joinRoom(room, spectate); } else if (!room_code_.isEmpty()) joinRoom(room_code_, spectating_); }
        if (type == "room.snapshot" || type == "game.snapshot" || type == "game.updated" || type == "game.finished" || type == "clock.updated" || type == "match.found") { room_code_ = event_payload.value("roomCode").toString(room_code_); emit roomChanged(); emit gameSnapshotReceived(event_payload.toVariantMap()); }
        if (type == "chat.message") emit chatReceived(event_payload.value("nickname").toString(), event_payload.value("message").toString());
        if (type == "error") { const auto error = event_payload.value("message").toString("实时请求失败"); setStatus(error); emit requestFailed(error); }
        emit realtimeEvent(type, event_payload.toVariantMap());
    }
}

void NetworkClient::sendRealtimeJson(QJsonObject object) { if (!websocket_ready_) return; object["sequence"] = ++outgoing_sequence_; sendWebSocketFrame(QJsonDocument(object).toJson(QJsonDocument::Compact)); }
void NetworkClient::sendWebSocketFrame(const QByteArray& payload) {
    if (payload.size() > 65535) { emit requestFailed("实时消息过长"); return; }
    QByteArray frame; frame.append(char(0x81)); if (payload.size() < 126) frame.append(char(0x80 | payload.size())); else { frame.append(char(0x80 | 126)); frame.append(char((payload.size() >> 8) & 0xff)); frame.append(char(payload.size() & 0xff)); }
    QByteArray mask(4, Qt::Uninitialized); for (auto& byte : mask) byte = static_cast<char>(QRandomGenerator::global()->generate() & 0xff); frame += mask;
    for (int index = 0; index < payload.size(); ++index) frame.append(payload[index] ^ mask[index % 4]); socket_.write(frame);
}

void NetworkClient::createRoom(int boardSize, bool isPublic, int mainTimeSeconds, int byoYomiSeconds) {
    if (!loggedIn()) { emit requestFailed("请先登录"); return; }
    QNetworkRequest request{QUrl(server_url_ + "/api/v1/rooms")}; request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"); request.setRawHeader("Authorization", "Bearer " + token_.toUtf8());
    const auto reply = network_.post(request, QJsonDocument(QJsonObject{{"boardSize", boardSize}, {"isPublic", isPublic}, {"mainTimeSeconds", mainTimeSeconds}, {"byoYomiSeconds", byoYomiSeconds}}).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply] { const auto document = QJsonDocument::fromJson(reply->readAll()); const auto object = document.object(); reply->deleteLater(); if (!object.value("ok").toBool()) { emit requestFailed(object.value("error").toString("创建房间失败")); return; } joinRoom(object.value("data").toObject().value("roomCode").toString()); });
}

void NetworkClient::joinRoom(const QString& roomCode, bool spectate) {
    if (roomCode.trimmed().isEmpty()) { emit requestFailed("请输入六位房间码"); return; }
    if (!realtime_authenticated_) { pending_room_ = roomCode; pending_spectate_ = spectate; connectRealtime(); setStatus("实时连接建立后自动进入房间"); return; }
    spectating_ = spectate; room_code_ = roomCode.trimmed().toUpper(); emit roomChanged();
    sendRealtimeJson({{"type", spectate ? "spectate.join" : "room.join"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{{"roomCode", room_code_}}}, {"sequence", last_sequence_}});
}

void NetworkClient::leaveRoom() { if (!room_code_.isEmpty()) sendRealtimeJson({{"type", "room.leave"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{}}, {"sequence", last_sequence_}}); room_code_.clear(); spectating_ = false; emit roomChanged(); }
void NetworkClient::playMove(int x, int y) { if (spectating_) { emit requestFailed("观战者不能落子"); return; } sendRealtimeJson({{"type", "game.move"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{{"x", x}, {"y", y}}}, {"sequence", last_sequence_}}); }
void NetworkClient::pass() { if (!spectating_) sendRealtimeJson({{"type", "game.pass"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{}}, {"sequence", last_sequence_}}); }
void NetworkClient::resign() { if (!spectating_) sendRealtimeJson({{"type", "game.resign"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{}}, {"sequence", last_sequence_}}); }
void NetworkClient::sendChat(const QString& content) { if (content.trimmed().isEmpty()) return; sendRealtimeJson({{"type", "chat.send"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{{"content", content.trimmed()}}}, {"sequence", last_sequence_}}); }
void NetworkClient::joinMatch(int boardSize) { if (!realtime_authenticated_) { pending_match_board_ = boardSize; connectRealtime(); setStatus("正在建立实时连接并开始匹配"); return; } sendRealtimeJson({{"type", "match.join"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{{"boardSize", boardSize}}}, {"sequence", last_sequence_}}); }
void NetworkClient::cancelMatch() { if (websocket_ready_) sendRealtimeJson({{"type", "match.cancel"}, {"requestId", QUuid::createUuid().toString(QUuid::WithoutBraces)}, {"payload", QJsonObject{}}, {"sequence", last_sequence_}}); }
void NetworkClient::loadGameRecords() {
    QNetworkRequest request{QUrl(server_url_ + "/api/v1/games")}; request.setRawHeader("Authorization", "Bearer " + token_.toUtf8()); const auto reply=network_.get(request);
    connect(reply,&QNetworkReply::finished,this,[this,reply]{ const auto document=QJsonDocument::fromJson(reply->readAll()); const auto object=document.object(); reply->deleteLater(); if(!object.value("ok").toBool()){ emit requestFailed(object.value("error").toString("无法加载棋谱")); return; } QVariantList records; for(const auto& row:object.value("data").toArray()) records.append(row.toObject().toVariantMap()); emit recordsReceived(records); });
}
void NetworkClient::loadSgf(int gameId) {
    QNetworkRequest request{QUrl(server_url_ + "/api/v1/games/" + QString::number(gameId) + "/sgf")}; request.setRawHeader("Authorization", "Bearer " + token_.toUtf8()); const auto reply=network_.get(request);
    connect(reply,&QNetworkReply::finished,this,[this,reply]{ const auto document=QJsonDocument::fromJson(reply->readAll()); const auto object=document.object(); reply->deleteLater(); if(!object.value("ok").toBool()){ emit requestFailed(object.value("error").toString("无法加载 SGF")); return; } emit sgfReceived(object.value("data").toObject().value("sgf").toString()); });
}
void NetworkClient::createAiGame(int boardSize, int mainTimeSeconds, int byoYomiSeconds) {
    if (!loggedIn()) { emit requestFailed("请先登录"); return; }
    QNetworkRequest request{QUrl(server_url_ + "/api/v1/ai/games")}; request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json"); request.setRawHeader("Authorization","Bearer "+token_.toUtf8());
    const auto reply=network_.post(request,QJsonDocument(QJsonObject{{"boardSize",boardSize},{"mainTimeSeconds",mainTimeSeconds},{"byoYomiSeconds",byoYomiSeconds}}).toJson(QJsonDocument::Compact));
    connect(reply,&QNetworkReply::finished,this,[this,reply]{const auto document=QJsonDocument::fromJson(reply->readAll()); const auto object=document.object(); reply->deleteLater(); if(!object.value("ok").toBool()){emit requestFailed(object.value("error").toString("创建人机对局失败"));return;} joinRoom(object.value("data").toObject().value("roomCode").toString());});
}
