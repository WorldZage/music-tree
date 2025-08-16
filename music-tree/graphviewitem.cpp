#include "graphviewitem.h"
#include <QPainter>

GraphViewItem::GraphViewItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject); // better perf
    setAntialiasing(true);

    connect(&timer, &QTimer::timeout, this, &GraphViewItem::updateLayout);
    timer.start(30); // ~33 FPS
}

/*
void GraphViewItem::setNodes(const QList<QPointF> &nodes) {
    if (m_nodes != nodes) {
        m_nodes = nodes;
        emit nodesChanged();
        update();
    }
}

void GraphViewItem::setEdges(const QList<QLineF> &edges) {
    if (m_edges != edges) {
        m_edges = edges;
        emit edgesChanged();
        update();
    }
}

*/


void GraphViewItem::addArtistNode(const QString &name) {
    if (artistIndex.contains(name)) return;

    ArtistNode node;
    node.name = name;
    node.pos = QPointF(QRandomGenerator::global()->bounded(width()),
                       QRandomGenerator::global()->bounded(height()));
    node.velocity = QPointF(0, 0);

    int idx = nodes.size();
    nodes.push_back(node);
    artistIndex.insert(name, idx);
}


void GraphViewItem::addCollaboration(const QStringList &artistNames) {
    for (int i = 0; i < artistNames.size(); ++i) {
        for (int j = i + 1; j < artistNames.size(); ++j) {
            int ai = artistIndex.value(artistNames[i], -1);
            int aj = artistIndex.value(artistNames[j], -1);
            if (ai < 0 || aj < 0) continue;

            QPair<int,int> key = qMakePair(std::min(ai,aj), std::max(ai,aj));
            collaborationCount[key] += 1;
        }
    }
}

void GraphViewItem::finalizeGraphLayout() {
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


void GraphViewItem::updateLayout() {
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

        // Keep inside bounds
        n.pos.setX(std::clamp(n.pos.x(), 0.0, width()));
        n.pos.setY(std::clamp(n.pos.y(), 0.0, height()));
    }

    update(); // trigger repaint
}

void GraphViewItem::paint(QPainter *painter)
{

    painter->setRenderHint(QPainter::Antialiasing);

    // Draw edges
    for (const auto &e : std::as_const(edges)) {
        painter->setPen(QPen(Qt::gray, std::min(8.0, 1.0 + e.sharedCount*0.5)));
        painter->drawLine(nodes[e.a].pos, nodes[e.b].pos);
    }

    // Draw nodes
    for (const auto &n : std::as_const(nodes)) {
        double r = 20.0;
        painter->setBrush(Qt::white);
        painter->setPen(Qt::black);
        painter->drawEllipse(n.pos, r, r);
        painter->drawText(n.pos.x()-r/2, n.pos.y()-r-5, n.name);
    }
}



