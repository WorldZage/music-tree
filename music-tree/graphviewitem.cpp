#include "graphviewitem.h"
#include <QPainter>

GraphViewItem::GraphViewItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject); // better perf
    setAntialiasing(true);

    // setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptedMouseButtons(Qt::AllButtons); // allow clicks
    setAcceptHoverEvents(true);   // For hover without buttons
    setFlag(ItemHasContents, true);
    setFlag(ItemAcceptsInputMethod, true);
    setFlag(ItemIsFocusScope, true); // optional, sometimes needed for key/mouse combo
    setAcceptTouchEvents(true);
    setKeepMouseGrab(true);


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
    for (auto [nKey, node] : nodeData.asKeyValueRange()) {
        if (ui.mode == UiContext::Mode::DraggingNode && nKey == ui.activeNodeId)
            continue; // skip layout forces for this node
        node.pos += node.velocity;
        node.velocity *= 0.85;

        // Constrain nodes fully inside the QML box
        node.pos.setX(std::clamp(node.pos.x(), nodeRadius + boundaryThreshold, w - (nodeRadius + boundaryThreshold)));
        node.pos.setY(std::clamp(node.pos.y(), nodeRadius + boundaryThreshold, h - (nodeRadius + boundaryThreshold)));
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

// ###
// Mouse events:
// ###

bool GraphViewItem::event(QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseButtonPress: {
        auto *me = static_cast<QMouseEvent*>(ev);
        // qDebug() << "Mouse press at" << me->pos();
        mousePressEvent(me);
        return true; // consume event
    }
    case QEvent::MouseMove: {
        auto *me = static_cast<QMouseEvent*>(ev);
        // qDebug() << "Mouse move at" << me->pos();
        mouseMoveEvent(me);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto *me = static_cast<QMouseEvent*>(ev);
        // qDebug() << "Mouse release at" << me->pos();
        mouseReleaseEvent(me);
        return true;
    }

    default:
        break;
    }

    // Fallback to base implementation for unhandled events
    return QQuickPaintedItem::event(ev);
}


void GraphViewItem::mousePressEvent(QMouseEvent *event) {
    event->accept();

    QString hitId = hitTestNode(event->pos());
    if (!hitId.isEmpty() && event->button() == Qt::LeftButton) {
        // Start dragging
        ui.mode = UiContext::Mode::DraggingNode;
        ui.activeNodeId = hitId;
        ui.dragStartPos = event->position();
        ui.dragOffset = nodeData[hitId].pos - event->position(); // anchor offset
        qDebug() << "Dragging node started:" << hitId;
    } else if (!hitId.isEmpty() && event->button() == Qt::RightButton) {
        // Right click, future context menu
        qDebug() << "Right-clicked node:" << hitId;
    }
}

void GraphViewItem::mouseMoveEvent(QMouseEvent *event) {
    event->accept();

    if (ui.mode == UiContext::Mode::DraggingNode && !ui.activeNodeId.isEmpty()) {
        // Move node relative to original drag offset
        QPointF newPos = event->position() + ui.dragOffset;

        // Constrain node within widget bounds
        const double w = width();
        const double h = height();
        newPos.setX(std::clamp(newPos.x(), nodeRadius + boundaryThreshold, w - nodeRadius - boundaryThreshold));
        newPos.setY(std::clamp(newPos.y(), nodeRadius + boundaryThreshold, h - nodeRadius - boundaryThreshold));

        nodeData[ui.activeNodeId].pos = newPos;
        nodeData[ui.activeNodeId].velocity = QPointF(0, 0); // stop passive-layout fighting
        update(); // trigger repaint
    }
}

void GraphViewItem::mouseReleaseEvent(QMouseEvent *event) {
    event->accept();

    if (ui.mode == UiContext::Mode::DraggingNode && event->button() == Qt::LeftButton) {
        // qDebug() << "Dragging node ended:" << ui.activeNodeId;
    }
    ui.activeNodeId.clear();
    ui.mode = UiContext::Mode::None;
}


QString GraphViewItem::hitTestNode(const QPointF &pos) const {
    for (auto it = nodeData.begin(); it != nodeData.end(); ++it) {
        double dx = pos.x() - it.value().pos.x();
        double dy = pos.y() - it.value().pos.y();
        if (std::hypot(dx, dy) <= nodeRadius) {
            return it.key(); // return artistId
        }
    }
    return {};
}


