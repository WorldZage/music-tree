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

void GraphViewItem::addArtistNode(const Artist& sessionArtist) {

    if (nodeData.contains(sessionArtist.id)) return;

    ArtistNode node;
    node.name = sessionArtist.name;
    node.pos = QPointF(QRandomGenerator::global()->bounded(width()),
                       QRandomGenerator::global()->bounded(height()));
    node.velocity = QPointF(0, 0);

    nodeData.insert(sessionArtist.id, node);
}

void GraphViewItem::removeArtistNode(const Artist& sessionArtist) {
    for (auto it = nodeData.begin(), end = nodeData.end(); it!=end; ++it) {
        if (it.key() == sessionArtist.id) {
            it = nodeData.erase(it);
            break;
        }
    }
}

void GraphViewItem::finalizeGraphLayout() {
    const QVector<Artist> artists = m_artistService->sessionArtists();

    for (auto it = nodeData.begin(); it != nodeData.end(); ) {
        auto artistStillInSession = std::find_if(artists.begin(), artists.end(),
                                                 [&it](const Artist& a){ return a.id == it.key(); });

        if (artistStillInSession == artists.end()) {
            it = nodeData.erase(it); // erase returns next iterator
        } else {
            ++it;
        }
    }
    /*
    edges.clear();
    edges.reserve(collaborationCount.size());
    for (auto it = collaborationCount.begin(); it != collaborationCount.end(); ++it) {
        Edge e;
        e.a = it.key().first;
        e.b = it.key().second;
        e.sharedCount = it.value();
        edges.push_back(e);
    }
    */
}


void GraphViewItem::updateLayout() {
    // Repulsion
    const QVector<Artist> artists = m_artistService->sessionArtists();
    for (auto itA = artists.begin(); itA != artists.end(); ++itA) {
        for (auto itB = itA; itB != artists.end(); ++itB) {

            QPointF delta = nodeData[itA->id].pos - nodeData[itB->id].pos;
                //nodes[i].pos - nodes[j].pos;
            double dist = std::max(1.0, std::hypot(delta.x(), delta.y()));
            QPointF dir = delta / dist;
            double force = repulsion / (dist * dist);
            nodeData[itA->id].velocity += dir * force;
            nodeData[itB->id].velocity -= dir * force;
        }
    }

    // Springs
    const SessionCollaborations collabs = m_artistService->collabs();
    for (const auto &e : std::as_const(collabs)) {
        QString artistId1 = e.first.a, artistId2 = e.first.b;

        QPointF delta = nodeData[artistId1].pos - nodeData[artistId2].pos;
        double dist = std::max(1e-6, std::hypot(delta.x(), delta.y()));
        QPointF dir = delta / dist;
        double force = springStrength * (dist - springLength);
        nodeData[artistId1].velocity -= dir * force;
        nodeData[artistId2].velocity += dir * force;
    }

    const double w = width();
    const double h = height();

    // Integrate + damping
    for (auto &n : nodeData) {
        n.pos += n.velocity;
        n.velocity *= 0.85;

        // Constrain nodes fully inside the QML box
        n.pos.setX(std::clamp(n.pos.x(), nodeRadius + boundaryThreshold, w - (nodeRadius + boundaryThreshold)));
        n.pos.setY(std::clamp(n.pos.y(), nodeRadius + boundaryThreshold, h - (nodeRadius + boundaryThreshold)));
    }

    update(); // trigger repaint
}

void GraphViewItem::paint(QPainter *painter)
{

    painter->setRenderHint(QPainter::Antialiasing);

    const SessionCollaborations collabs = m_artistService->collabs();
    // Draw edges
    for (const auto &e : std::as_const(collabs)) {
        QString artistId1 = e.first.a, artistId2 = e.first.b;
        int sharedReleasesCount = e.second.size();

        painter->setPen(QPen(Qt::gray, std::min(8.0, 1.0 + sharedReleasesCount *0.5)));
        painter->drawLine(nodeData[artistId1].pos, nodeData[artistId2].pos);
    }

    // Draw nodes
    for (const auto &n : std::as_const(nodeData)) {

        painter->setBrush(Qt::white);
        painter->setPen(Qt::black);
        painter->drawEllipse(n.pos, nodeRadius, nodeRadius);
        painter->drawText(n.pos.x()-nodeRadius/2, n.pos.y()-nodeRadius-5, n.name);
    }
}

void GraphViewItem::setArtistService(ArtistService *artistService) {
    m_artistService = artistService;
    this->connectSessionEvents(m_artistService->sessionManager());

}

void GraphViewItem::connectSessionEvents(const SessionManager *sessionManager) {
    QObject::connect(sessionManager, &SessionManager::artistAdded,
                    this, [this](const Artist& artist){
                        this->addArtistNode(artist);

                     });
    QObject::connect(sessionManager, &SessionManager::artistRemoved,
                    this, [this](const Artist& artist){
                        this->removeArtistNode(artist);
                        this->finalizeGraphLayout();
                    });

}




