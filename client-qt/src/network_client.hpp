#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QTcpSocket>
#include <QTimer>

#include <functional>

// Minimal RFC 6455 text client for the application's ws:// LAN endpoint.
// Keeping it on QTcpSocket avoids making Qt's optional WebSockets module a
// deployment prerequisite for the msvc2022_64 kit.
class NetworkClient final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString serverUrl READ serverUrl WRITE setServerUrl NOTIFY serverUrlChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY sessionChanged)
    Q_PROPERTY(QString nickname READ nickname NOTIFY sessionChanged)
    Q_PROPERTY(QString currentRoomCode READ currentRoomCode NOTIFY roomChanged)
    Q_PROPERTY(bool spectating READ spectating NOTIFY roomChanged)
    Q_PROPERTY(bool aiAvailable READ aiAvailable NOTIFY katagoStatusChanged)
    Q_PROPERTY(QString katagoReason READ katagoReason NOTIFY katagoStatusChanged)
public:
    explicit NetworkClient(QObject* parent = nullptr);
    QString serverUrl() const { return server_url_; }
    void setServerUrl(const QString& url);
    QString status() const { return status_; }
    bool loggedIn() const { return !token_.isEmpty(); }
    QString nickname() const { return nickname_; }
    QString currentRoomCode() const { return room_code_; }
    bool spectating() const { return spectating_; }
    bool aiAvailable() const { return ai_available_; }
    QString katagoReason() const { return katago_reason_; }
    Q_INVOKABLE void registerAccount(const QString& nickname, const QString& password);
    Q_INVOKABLE void login(const QString& nickname, const QString& password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void loadPublicRooms();
    Q_INVOKABLE void connectRealtime();
    Q_INVOKABLE void createRoom(int boardSize, bool isPublic, int mainTimeSeconds = 600, int byoYomiSeconds = 30);
    Q_INVOKABLE void joinRoom(const QString& roomCode, bool spectate = false);
    Q_INVOKABLE void leaveRoom();
    Q_INVOKABLE void playMove(int x, int y);
    Q_INVOKABLE void pass();
    Q_INVOKABLE void resign();
    Q_INVOKABLE void sendChat(const QString& content);
    Q_INVOKABLE void joinMatch(int boardSize);
    Q_INVOKABLE void cancelMatch();
    Q_INVOKABLE void loadGameRecords();
    Q_INVOKABLE void loadSgf(int gameId);
    Q_INVOKABLE void createAiGame(int boardSize, int mainTimeSeconds = 600, int byoYomiSeconds = 30);
    Q_INVOKABLE void loadKataGoStatus();
signals:
    void serverUrlChanged(); void statusChanged(); void sessionChanged(); void roomChanged();
    void roomsReceived(const QVariantList& rooms); void requestFailed(const QString& message);
    void realtimeEvent(const QString& type, const QVariantMap& payload);
    void gameSnapshotReceived(const QVariantMap& snapshot);
    void chatReceived(const QString& nickname, const QString& content);
    void recordsReceived(const QVariantList& records);
    void sgfReceived(const QString& sgf);
    void katagoStatusChanged();
private:
    void postJson(const QString& path, const QJsonObject& body, std::function<void(const QJsonObject&)> success);
    void setStatus(const QString& value);
    void consumeSocketData();
    void sendRealtimeJson(QJsonObject object);
    void sendWebSocketFrame(const QByteArray& payload);
    QNetworkAccessManager network_;
    QTcpSocket socket_;
    QByteArray socket_buffer_;
    bool websocket_ready_{};
    bool realtime_authenticated_{};
    int last_sequence_{};
    int outgoing_sequence_{};
    QTimer reconnect_timer_;
    QString server_url_{"http://127.0.0.1:8081"};
    QString token_; QString nickname_; QString status_{"尚未连接服务端"};
    QString room_code_; bool spectating_{};
    bool ai_available_{}; QString katago_reason_{"尚未检查 KataGo 状态"};
    QString pending_room_; bool pending_spectate_{}; int pending_match_board_{};
};
