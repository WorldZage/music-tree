#pragma once
#include <QObject>
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
#include "sessionmanager.h"
#include "artistservice.h"

struct ArtistNode {
    // TODO: SessionArtist artist;
    QString name;
    QPointF pos;
    QPointF velocity;
};

class GraphViewItem : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit GraphViewItem(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;
    void setArtistService(ArtistService *artistService);

private slots:
    void updateLayout();

private:
    void connectSessionEvents(const SessionManager *sessionManager);
    void addArtistNode(const Artist& sessionArtist);
    void removeArtistNode(const Artist& sessionArtist);
    void finalizeGraphLayout();


    QMap<QString, ArtistNode> nodeData; // artistId as key.
    //QMap<QString, int> artistIndex;
    //QMap<QPair<int, int>, int> collaborationCount;

    QTimer timer;
    double repulsion = 2000.0;
    double springLength = 100.0;
    double springStrength = 0.01;
    double nodeRadius = 20.0;
    double boundaryThreshold = 30.0;

    ArtistService* m_artistService;

};
