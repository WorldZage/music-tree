#pragma once
#include <QQuickPaintedItem>
#include <QQuickPaintedItem>
#include <QTimer>
#include <QPointF>
#include <QLineF>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QString>
#include <QRandomGenerator>

struct ArtistNode {
    QString name;
    QPointF pos;
    QPointF velocity;
};

struct Edge {
    int a, b;
    int sharedCount;
};

class GraphViewItem : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit GraphViewItem(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;

    Q_INVOKABLE void addArtistNode(const QString &name);
    Q_INVOKABLE void addCollaboration(const QStringList &artistNames);
    Q_INVOKABLE void finalizeGraphLayout();


private slots:
    void updateLayout();

private:
    QVector<ArtistNode> nodes;
    QVector<Edge> edges;
    QMap<QString, int> artistIndex;
    QMap<QPair<int, int>, int> collaborationCount;

    QTimer timer;
    double repulsion = 2000.0;
    double springLength = 100.0;
    double springStrength = 0.01;

};
