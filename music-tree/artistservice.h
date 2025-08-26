#pragma once

#include <QString>
#include <QFuture>
#include <optional>
#include <vector>
#include "artist.h"
#include "discogsmanager.h"
#include "databasemanager.h"
#include "sessionmanager.h"


class ArtistService : public QObject{
    Q_OBJECT

public:
    explicit ArtistService(QObject* parent = nullptr);

    // Entry point for UI: searches for an artist by name
    Q_INVOKABLE void searchByName(const QString& name);
    Q_INVOKABLE void clearDb(void);

    SessionManager m_session = SessionManager();

signals:
    void artistFound(const Artist& artist);                     // UI list update
    void collaborationsReady(const QMap<QString, std::vector<QString>>& collabs); // UI graph update


private slots:
    void onDiscogsDataReady(const Artist& artist);
    void onDiscogsArtistSearchReady(const std::vector<Artist>& artists);
    void onArtistFound(const Artist& artist);

private:
    void cacheArtist(const Artist& artist);
    // List all cached artists in DB
    std::vector<Artist> listCachedArtists() const;

    void updateReleasesFromJson(const QJsonArray &jsonReleases, const QString &artistId);
    std::vector<ReleaseInfo> parseReleasesJsonArray(const QJsonArray &releasesArray);

private:
    DiscogsManager m_discogs = DiscogsManager();
    DatabaseManager m_db = DatabaseManager();

};
