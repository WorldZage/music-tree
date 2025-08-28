// SessionManager.h
#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include "artist.h"




class SessionManager : public QObject {
    Q_OBJECT
public:
    SessionManager(QObject* parent = nullptr);

    const QVector<Artist>& artists() const { return m_artists; }
    const SessionCollaborations& collabs() const { return m_collabs; }

    void addArtist(const Artist& artist);


    void removeArtistByListIndex(const int listIndex);
    bool containsArtist(const Artist& artist);


    void clear();

signals:
    void artistAdded(const Artist& artist);
    void artistRemoved(const Artist& artist);
    void sessionCleared();

private:
    void removeArtistById(const QString& artistId);
    void updateCollabsForNewArtist(const Artist& newArtist);
    void removeCollabsForArtist(const QString& artistId);

    QVector<Artist> m_artists;
    SessionCollaborations m_collabs;
};
