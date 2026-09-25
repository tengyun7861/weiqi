#include "game_controller.hpp"

#include <QVariantMap>

GameController::GameController(QObject *parent) : QObject(parent), board_(19) {}

QVariantList GameController::stones() const {
    if (online_game_) return remote_stones_;
    QVariantList result;
    for (int row = 0; row < board_.size(); ++row) {
        for (int column = 0; column < board_.size(); ++column) {
            const auto stone = board_.at(column, row);
            if (stone == weiqi::Stone::empty) continue;
            QVariantMap point;
            point.insert("x", column);
            point.insert("y", row);
            point.insert("black", stone == weiqi::Stone::black);
            result.append(point);
        }
    }
    return result;
}

QString GameController::currentPlayer() const {
    if (online_game_) return remote_current_player_ == "white" ? "白棋" : "黑棋";
    return board_.current_player() == weiqi::Stone::black ? "黑棋" : "白棋";
}

void GameController::play(int column, int row) {
    if (online_game_) { if (finished_) { last_error_ = "对局已经结束"; emit boardChanged(); return; } for (const auto& point: remote_stones_) if (point.toMap().value("x").toInt()==column && point.toMap().value("y").toInt()==row) { last_error_="该位置已有棋子"; emit boardChanged(); return; } last_error_.clear(); emit moveRequested(column, row); return; }
    const auto result = board_.play(column, row);
    last_error_ = result.accepted ? QString{} : QString::fromStdString(result.error);
    emit boardChanged();
}

void GameController::pass() {
    if (online_game_) { if (finished_) { last_error_ = "对局已经结束"; emit boardChanged(); return; } emit passRequested(); return; }
    board_.pass();
    last_error_.clear();
    emit boardChanged();
}

void GameController::reset(int size) {
    try { board_ = weiqi::GoBoard(size); remote_stones_.clear(); online_game_ = false; finished_ = false; black_time_ = 0; white_time_ = 0; black_byo_remaining_=0; white_byo_remaining_=0; byo_yomi_seconds_=0; remote_current_player_.clear(); last_error_.clear(); }
    catch (const std::exception& error) { last_error_ = QString::fromUtf8(error.what()); }
    emit boardChanged();
}

void GameController::setOnlineGame(bool enabled) { online_game_ = enabled; if (!enabled) remote_stones_.clear(); emit boardChanged(); }

void GameController::applyServerSnapshot(const QVariantMap& snapshot) {
    const int size = snapshot.value("boardSize", board_.size()).toInt();
    try { if (board_.size() != size) board_ = weiqi::GoBoard(size); } catch (const std::exception& error) { last_error_ = QString::fromUtf8(error.what()); emit boardChanged(); return; }
    remote_stones_.clear(); const auto raw = snapshot.value("stones").toList();
    for (int index = 0; index < raw.size(); ++index) {
        const int stone = raw[index].toInt(); if (stone == 0) continue;
        QVariantMap point; point.insert("x", index % size); point.insert("y", index / size); point.insert("black", stone == 1); remote_stones_.append(point);
    }
    online_game_ = true; finished_ = snapshot.value("finished").toBool(); black_time_ = snapshot.value("blackTime").toInt(); white_time_ = snapshot.value("whiteTime").toInt(); black_byo_remaining_=snapshot.value("blackByoRemaining").toInt(); white_byo_remaining_=snapshot.value("whiteByoRemaining").toInt(); byo_yomi_seconds_=snapshot.value("byoYomiSeconds").toInt(); remote_current_player_=snapshot.value("currentPlayer").toString(); last_error_.clear(); emit boardChanged();
}
