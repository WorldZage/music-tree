// SessionManager.cpp
#include "sessionmanager.h"

SessionManager::SessionManager(QObject* parent) : QObject(parent) {}

void SessionManager::addArtist(const SessionArtist& sessionArtist) {
    m_artists.append(sessionArtist);
    emit artistAdded(sessionArtist);
}
void SessionManager::addArtist(const QString& artistId, const QString& artistName) {
    SessionArtist sessionArtist = {artistId, artistName};
    m_artists.append(sessionArtist);
    emit artistAdded(sessionArtist);
}

void SessionManager::removeArtist(const QString& artistId) {
    auto it = std::remove_if(m_artists.begin(), m_artists.end(),
                             [&](const SessionArtist& a){ return a.id == artistId; });
    if (it != m_artists.end()) {
        m_artists.erase(it, m_artists.end());
        emit artistRemoved(artistId);
    }
}

void SessionManager::clear() {
    m_artists.clear();
    emit sessionCleared();
}
