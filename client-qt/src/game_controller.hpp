#pragma once

#include "weiqi/go_board.hpp"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class GameController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList stones READ stones NOTIFY boardChanged)
    Q_PROPERTY(QString currentPlayer READ currentPlayer NOTIFY boardChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY boardChanged)
    Q_PROPERTY(int boardSize READ boardSize NOTIFY boardChanged)
    Q_PROPERTY(int blackTime READ blackTime NOTIFY boardChanged)
    Q_PROPERTY(int whiteTime READ whiteTime NOTIFY boardChanged)
    Q_PROPERTY(int blackByoRemaining READ blackByoRemaining NOTIFY boardChanged)
    Q_PROPERTY(int whiteByoRemaining READ whiteByoRemaining NOTIFY boardChanged)
    Q_PROPERTY(int byoYomiSeconds READ byoYomiSeconds NOTIFY boardChanged)
    Q_PROPERTY(bool finished READ finished NOTIFY boardChanged)
    Q_PROPERTY(bool onlineGame READ onlineGame NOTIFY boardChanged)
public:
    explicit GameController(QObject *parent = nullptr);
    QVariantList stones() const;
    QString currentPlayer() const;
    QString lastError() const { return last_error_; }
    int boardSize() const { return board_.size(); }
    int blackTime() const { return black_time_; }
    int whiteTime() const { return white_time_; }
    int blackByoRemaining() const { return black_byo_remaining_; }
    int whiteByoRemaining() const { return white_byo_remaining_; }
    int byoYomiSeconds() const { return byo_yomi_seconds_; }
    bool finished() const { return finished_; }
    bool onlineGame() const { return online_game_; }

    Q_INVOKABLE void play(int column, int row);
    Q_INVOKABLE void pass();
    Q_INVOKABLE void reset(int size = 19);
    Q_INVOKABLE void applyServerSnapshot(const QVariantMap& snapshot);
    Q_INVOKABLE void setOnlineGame(bool enabled);
signals:
    void boardChanged();
    void moveRequested(int column, int row);
    void passRequested();
private:
    weiqi::GoBoard board_;
    QString last_error_;
    QVariantList remote_stones_;
    bool online_game_{};
    bool finished_{};
    int black_time_{};
    int white_time_{};
    int black_byo_remaining_{};
    int white_byo_remaining_{};
    int byo_yomi_seconds_{};
    QString remote_current_player_;
};
