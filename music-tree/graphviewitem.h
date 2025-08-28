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



struct UiContext {
    enum class Mode {
        None,
        DraggingNode,
        PanningView,
        SelectingArea,
        ContextMenu
    };

    Mode mode = Mode::None;
    QString activeNodeId;
    QPointF dragStartPos;
    QPointF dragOffset;
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

    // mouse events:
    bool event(QEvent *ev) override;
    QString hitTestNode(const QPointF &pos) const;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    QMap<QString, ArtistNode> nodeData; // artistId as key.


    QTimer timer;
    double repulsion = 2000.0;
    double springLength = 100.0;
    double springStrength = 0.01;
    double nodeRadius = 20.0;
    double boundaryThreshold = 30.0;

    ArtistService* m_artistService;

    UiContext ui;

};
