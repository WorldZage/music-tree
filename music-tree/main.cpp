#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWidget>
#include <QWidget>

#include "discogsmanager.h"
#include "databasemanager.h"
#include "graphviewitem.h"
#include "artistservice.h"
#include "sessionartistmodel.h"




int main(int argc, char *argv[])
{
    //QGuiApplication app(argc, argv);
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

    // Core managers


    // Facade service
    ArtistService artistService = ArtistService();
    engine.rootContext()->setContextProperty("artistService", &artistService);

    // Register GraphViewWidget so we can create it from QML
    qmlRegisterType<GraphViewItem>("appmusictree", 1, 0, "GraphView");



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
        // (Optional) connect service signals to graph, if we expose such from ArtistService
        // Future feature, If ArtistService later emits an artistLoaded signal
        /*
        QObject::connect(&discogs, &DiscogsManager::artistAdded,
                         graph, [graph](const QString &name, const QStringList &collabs){
                             graph->addArtistNode(name);
                             for (const QString &c : collabs) {
                                 graph->addCollaboration({name, c});
                             }
                             graph->finalizeGraphLayout();
                         });
        */
    }


    return app.exec();
}
