// SessionManager.cpp
#include "sessionmanager.h"


SessionManager::SessionManager(QObject* parent) : QObject(parent) {
}


void SessionManager::loadArtistsFromFile() {
    QString fileName = QFileDialog::getOpenFileName(nullptr, tr("Open File"),
                                                    "/home",
                                                    tr("XML files (*.xml)"));
    qDebug() << "selected file name: " << fileName;

}

void SessionManager::addArtist(const Artist& artist) {
    if (containsArtist(artist)) {
        qDebug() << "Artist already exists:" << artist.name;
        return;
    }

    m_artists.append(artist);
    registerArtistReleases(artist);
    updateCollabsForNewArtist(artist);

    emit artistAdded(artist);
}

void SessionManager::removeArtistByListIndex(const int listIndex) {
    if (listIndex < 0 || listIndex >= m_artists.size()) {
        qWarning() << "removeArtistByListIndex: index out of range:" << listIndex;
        return;
    }
    const QString id = m_artists.at(listIndex).id;
    qDebug() << "remove artist of" << m_artists.at(listIndex).name
             << "(id:" << id << ") at index:" << listIndex;
    removeArtistById(id); // single authority
}

void SessionManager::removeArtistById(const QString& artistId) {
    removeCollabsForArtist(artistId);

    QMutableVectorIterator<Artist> it(m_artists);
    while (it.hasNext()) {
        if (it.next().id == artistId) {
            unregisterArtistReleases(it.value());
            const Artist removed = it.value();  // snapshot before remove()
            it.remove();                        // safe; iterator now before the next item

            emit artistRemoved(removed);
            return;
        }
    }

    qWarning() << "removeArtistById: id not found:" << artistId;
}

void SessionManager::removeCollabsForArtist(const QString& artistId) {
    for (auto it = m_collabs.begin(); it != m_collabs.end(); ) {
        if (it->first.a == artistId || it->first.b == artistId) {
            it = m_collabs.erase(it);
        } else {
            ++it;
        }
    }
}

bool SessionManager::containsArtist(const Artist& artist) {
    for (const Artist& a : std::as_const(m_artists)) {
        if (a.id == artist.id) return true;
    }
    return false;
}

void SessionManager::registerArtistReleases(const Artist& artist) {
    for (const ReleaseInfo& r : artist.releases) {
        m_releaseToArtists.insert(r.id, artist.id);
    }
}

void SessionManager::unregisterArtistReleases(const Artist& artist) {
    for (const ReleaseInfo& r : artist.releases) {
        m_releaseToArtists.remove(r.id, artist.id);
    }
}

// Debug/query: who owns a release?
QVector<QString> SessionManager::getArtistsForRelease(const QString& releaseId) const {
    return m_releaseToArtists.values(releaseId).toVector();
}


void SessionManager::updateCollabsForNewArtist(const Artist& newArtist) {
    for (const ReleaseInfo& rel : newArtist.releases) {
        const auto others = m_releaseToArtists.values(rel.id);
        for (const QString& otherId : others) {
            if (otherId == newArtist.id) continue;

            CollabKey key(newArtist.id, otherId);
            ArtistCollaboration& collab = m_collabs[key]; // inserts if missing
            if (!collab.contains(rel.id)) {
                collab.append(rel.id);
                qDebug() << "Release match:" << rel.title << "; with artistId:" << otherId;
            }
        }
    }
}



void SessionManager::clear() {
    m_artists.clear();
    m_collabs.clear();
    emit sessionCleared();
}
