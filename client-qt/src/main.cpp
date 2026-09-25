#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "game_controller.hpp"
#include "network_client.hpp"
#include "replay_controller.hpp"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("弈境围棋");
    QGuiApplication::setApplicationName("弈境围棋");

    QQmlApplicationEngine engine;
    GameController game;
    NetworkClient network;
    ReplayController replay;
    QObject::connect(&network, &NetworkClient::gameSnapshotReceived, &game, &GameController::applyServerSnapshot);
    QObject::connect(&game, &GameController::moveRequested, &network, &NetworkClient::playMove);
    QObject::connect(&game, &GameController::passRequested, &network, &NetworkClient::pass);
    engine.rootContext()->setContextProperty("game", &game);
    engine.rootContext()->setContextProperty("network", &network);
    engine.rootContext()->setContextProperty("replay", &replay);
    QObject::connect(&network, &NetworkClient::sgfReceived, &replay, &ReplayController::loadSgf);
    engine.loadFromModule("Weiqi", "Main");
    if (engine.rootObjects().isEmpty()) return 1;
    return app.exec();
}
