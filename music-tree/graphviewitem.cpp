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

    const double w = width();
    const double h = height();

    // Integrate + damping
    for (auto &n : nodes) {
        n.pos += n.velocity;
        n.velocity *= 0.85;

        // Keep inside bounds
        /*
        n.pos.setX(std::clamp(n.pos.x(), 0.0, width()));
        n.pos.setY(std::clamp(n.pos.y(), 0.0, height()));
        */
        // Constrain nodes fully inside the QML box
        n.pos.setX(std::clamp(n.pos.x(), nodeRadius, w - nodeRadius));
        n.pos.setY(std::clamp(n.pos.y(), nodeRadius, h - nodeRadius));
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

        painter->setBrush(Qt::white);
        painter->setPen(Qt::black);
        painter->drawEllipse(n.pos, nodeRadius, nodeRadius);
        painter->drawText(n.pos.x()-nodeRadius/2, n.pos.y()-nodeRadius-5, n.name);
    }
}



