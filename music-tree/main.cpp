#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "discogsmanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    DiscogsManager discogs;
    engine.rootContext()->setContextProperty("discogsManager", &discogs);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("music-tree", "Main");

    return app.exec();
}
