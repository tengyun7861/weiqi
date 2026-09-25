#pragma once

#include "weiqi/go_board.hpp"
#include "weiqi/sgf.hpp"

#include <QObject>
#include <QVariantList>

class ReplayController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList stones READ stones NOTIFY changed)
    Q_PROPERTY(int boardSize READ boardSize NOTIFY changed)
    Q_PROPERTY(int step READ step NOTIFY changed)
    Q_PROPERTY(int totalSteps READ totalSteps NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
public:
    explicit ReplayController(QObject* parent = nullptr);
    QVariantList stones() const;
    int boardSize() const { return board_size_; }
    int step() const { return step_; }
    int totalSteps() const { return static_cast<int>(moves_.size()); }
    QString error() const { return error_; }
    Q_INVOKABLE void loadSgf(const QString& sgf);
    Q_INVOKABLE void previous();
    Q_INVOKABLE void next();
    Q_INVOKABLE void jumpTo(int step);
signals:
    void changed();
private:
    void rebuild();
    int board_size_{19}; int step_{};
    std::vector<weiqi::SgfMove> moves_;
    weiqi::GoBoard board_{19}; QString error_;
};
