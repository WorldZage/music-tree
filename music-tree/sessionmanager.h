// SessionManager.h
#pragma once
#include <QObject>
#include <QString>
#include <QVector>
#include <QDebug>
#include <QFileDialog>
#include <QMutex>
#include "artist.h"




class SessionManager : public QObject {
    Q_OBJECT
public:
    SessionManager(QObject* parent = nullptr);

    const QVector<Artist>& artists() const { return m_artists; }
    const SessionCollaborations& collabs() const { return m_collabs; }

    bool containsArtist(const Artist& artist);
    const Artist* getArtistById(const QString& artistId);
    void addArtist(const Artist& artist);
    void removeArtistById(const QString& artistId);

    // debugging / queries
    QVector<QString> getArtistsForRelease(const QString& releaseId) const;

    void clear();

signals:
    void artistAdded(const Artist& artist);
    void artistRemoved(const Artist& artist);
    void sessionCleared();


private:

    void updateCollabsForNewArtist(const Artist& newArtist);
    void removeCollabsForArtist(const QString& artistId);

    void registerArtistReleases(const Artist& artist);
    void unregisterArtistReleases(const Artist& artist);


    QVector<Artist> m_artists;
    SessionCollaborations m_collabs;
    QMultiHash<QString, QString> m_releaseToArtists;  // releaseId -> artistId(s)

    QMutex sessionMutex;
};
