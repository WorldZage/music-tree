#include <QApplication>                 // <-- use QApplication (Widgets)
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QPen>
#include <QBrush>
#include <QMap>
#include <QPair>
#include <QTimer>
#include <QRandomGenerator>             // <-- for bounded() instead of rand()
#include <algorithm>
#include <cmath>

#include "discogsmanager.h"

struct ArtistNode {
    QString name;
    QPointF pos;
    QPointF velocity;
};

struct Edge {
    int a, b; // indices in nodes
    int sharedCount;
};

class GraphScene : public QGraphicsScene {
    QVector<ArtistNode> nodes;
    QVector<Edge> edges;
    QMap<QString, int> artistIndex;
    QMap<QPair<int, int>, int> collaborationCount;

    QTimer timer;
    double repulsion = 2000.0;
    double springLength = 100.0;
    double springStrength = 0.01;

public:
    GraphScene(QObject *parent = nullptr) : QGraphicsScene(parent) {
        connect(&timer, &QTimer::timeout, this, &GraphScene::updateLayout);
        timer.start(30); // ~33 FPS
    }

    void addArtist(const QString &name) {
        if (artistIndex.contains(name)) return;
        ArtistNode node;
        node.name = name;
        node.pos = QPointF(QRandomGenerator::global()->bounded(400.0),
                           QRandomGenerator::global()->bounded(400.0));
        node.velocity = QPointF(0, 0);
        int idx = nodes.size();
        nodes.push_back(node);
        artistIndex.insert(name, idx);
    }

    void addReleaseCollaboration(const QStringList &artistNames) {
        for (int i = 0; i < artistNames.size(); ++i) {
            for (int j = i + 1; j < artistNames.size(); ++j) {
                int ai = artistIndex.value(artistNames[i], -1);
                int aj = artistIndex.value(artistNames[j], -1);
                if (ai < 0 || aj < 0) continue; // safety guard

                QPair<int, int> key = qMakePair(std::min(ai, aj), std::max(ai, aj));
                collaborationCount[key] += 1;
            }
        }
    }

    void finalizeGraph() {
        edges.clear();
        edges.reserve(collaborationCount.size());
        for (auto it = collaborationCount.begin(); it != collaborationCount.end(); ++it) {
            Edge e;
            e.a = it.key().first;
            e.b = it.key().second;
            e.sharedCount = it.value();
            edges.push_back(e);
        }
    }

private:
    void updateLayout() {
        // Repulsion
        for (int i = 0; i < nodes.size(); ++i) {
            for (int j = i + 1; j < nodes.size(); ++j) {
                QPointF delta = nodes[i].pos - nodes[j].pos;
                double dist = std::max(1.0, std::hypot(delta.x(), delta.y()));
                QPointF dir = delta / dist;
                double force = repulsion / (dist * dist);
                nodes[i].velocity += dir * force;
                nodes[j].velocity -= dir * force;
            }
        }

        // Springs
        for (const auto &e : std::as_const(edges)) {
            QPointF delta = nodes[e.a].pos - nodes[e.b].pos;
            double dist = std::max(1e-6, std::hypot(delta.x(), delta.y()));
            QPointF dir = delta / dist;
            double force = springStrength * (dist - springLength);
            nodes[e.a].velocity -= dir * force;
            nodes[e.b].velocity += dir * force;
        }

        // Integrate + damping
        for (auto &n : nodes) {
            n.pos += n.velocity;
            n.velocity *= 0.85;
        }

        redraw();
    }

    void redraw() {
        clear();

        // Edges
        for (const auto &e : edges) {
            QPen pen(Qt::gray);
            pen.setWidthF(std::min(8.0, 1.0 + e.sharedCount * 0.5));
            addLine(nodes[e.a].pos.x(), nodes[e.a].pos.y(),
                    nodes[e.b].pos.x(), nodes[e.b].pos.y(), pen);
        }

        // Nodes
        for (const auto &n : nodes) {
            double r = 20;
            addEllipse(n.pos.x() - r, n.pos.y() - r, r*2, r*2,
                       QPen(Qt::black), QBrush(Qt::white));
            auto *label = addText(n.name);
            label->setPos(n.pos.x() - label->boundingRect().width() / 2,
                          n.pos.y() - r - 15);
        }
    }
};

int main(int argc, char *argv[])
{
    //QGuiApplication app(argc, argv);
    QApplication app(argc, argv);
    QQmlApplicationEngine engine;

    DiscogsManager discogs;
    engine.rootContext()->setContextProperty("discogsManager", &discogs);

    GraphScene *scene = new GraphScene;
    QGraphicsView *view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setWindowTitle("Artist Collaboration Graph");
    view->resize(800, 600);
    view->show();

    QObject::connect(&discogs, &DiscogsManager::artistAdded,
                     [scene](const QString &name, const QStringList &collabs){
                         scene->addArtist(name);
                         for (const QString &c : collabs) {
                             scene->addReleaseCollaboration({name, c});
                         }
                         scene->finalizeGraph();
                     });

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    //engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    engine.loadFromModule("music-tree", "Main");

    return app.exec();
}
