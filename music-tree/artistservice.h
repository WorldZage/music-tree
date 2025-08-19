#pragma once

#include <QString>
#include <QFuture>
#include <optional>
#include <vector>
#include "artist.h"
#include "discogsmanager.h"
#include "databasemanager.h"


class ArtistService : public QObject{
    Q_OBJECT

public:
    //explicit ArtistService(QObject *parent = nullptr);
    ArtistService(DiscogsManager* discogs, DatabaseManager* db);

    // Entry point for UI: searches for an artist by name
    Q_INVOKABLE QFuture<std::optional<Artist>> searchArtist(const QString& name);

    // Get full artist data (releases, metadata)
    QFuture<std::optional<Artist>> getArtistData(const QString& artistId);

    // List all cached artists in DB
    std::vector<Artist> listCachedArtists() const;

    void updateReleasesFromJson(const QJsonArray &jsonReleases, const QString &artistId);


private:
    void cacheArtist(const Artist& artist);

    std::vector<ReleaseInfo> parseReleasesJsonArray(const QJsonArray &releasesArray);

private:
    DiscogsManager* m_discogs;
    DatabaseManager* m_db;
};
