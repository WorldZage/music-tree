#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWidget>
#include <QWidget>

#include "discogsmanager.h"
#include "graphviewitem.h"



int main(int argc, char *argv[])
{
    //QGuiApplication app(argc, argv);
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;
    DiscogsManager discogs;
    engine.rootContext()->setContextProperty("discogsManager", &discogs);

    // Register GraphViewWidget so we can create it from QML
    //qmlRegisterType<QQuickWidget>("MyApp", 1, 0, "GraphWidget");
    qmlRegisterType<GraphViewItem>("MyApp", 1, 0, "GraphView");


    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("music-tree", "Main");
    if (engine.rootObjects().isEmpty()) return -1;


    // Find the GraphViewItem instance
    QObject *rootObj = engine.rootObjects().first();
    GraphViewItem *graph = rootObj->findChild<GraphViewItem*>("graph");
    if(!graph) {
        qWarning() << "GraphViewItem not found!";
    }
    else {
        QObject::connect(&discogs, &DiscogsManager::artistAdded,
                         graph, [graph](const QString &name, const QStringList &collabs){
                             graph->addArtistNode(name);
                             for (const QString &c : collabs) {
                                 graph->addCollaboration({name, c});
                             }
                             graph->finalizeGraphLayout();
                         });
    }


    return app.exec();
}
