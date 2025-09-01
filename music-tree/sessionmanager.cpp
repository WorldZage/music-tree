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
    m_artists.append(artist);
    emit artistAdded(artist);

    updateCollabsForNewArtist(artist);
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
            const Artist removed = it.value();  // snapshot before remove()
            it.remove();                        // safe; iterator now before the next item

            // Scan ahead for further duplicates of the *same* id
            QMutableVectorIterator<Artist> dupIt = it; // copy starting at current position
            while (dupIt.hasNext()) {
                if (dupIt.next().id == artistId) {
                    qWarning() << "Duplicate artist with id" << artistId
                               << "still present after first removal!";
                    break;
                }
            }

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
    auto it = std::find_if(m_artists.begin(), m_artists.end(), [&](const Artist& a){ return a.id == artist.id; });
    return (it != m_artists.end());
}


void SessionManager::updateCollabsForNewArtist(const Artist& newArtist) {
    // Iterate existing session artists
    for (const Artist& existing : std::as_const(m_artists)) {
        if (existing.id == newArtist.id) continue;

        // Find overlapping releases
        QVector<QString> shared;
        for (const ReleaseInfo& relNew : newArtist.releases) {
            for (const ReleaseInfo& relExisting : existing.releases) {
                if (relNew.id == relExisting.id) {
                    shared.append(relNew.id);
                    qDebug() << "Release match: " << relNew.title << "; with: " << existing.name;
                }
            }
        }

        if (!shared.isEmpty()) {
            CollabKey key(newArtist.id, existing.id);
            ArtistCollaboration& collab = m_collabs[key]; // inserts if missing
            for (const QString& r : shared) {
                if (!collab.contains(r)) {
                    collab.append(r);
                }
            }
        }
    }
}



void SessionManager::clear() {
    m_artists.clear();
    m_collabs.clear();
    emit sessionCleared();
}
