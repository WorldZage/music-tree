#include "artistservice.h"
#include "discogsmanager.h"
#include "databasemanager.h"

#include <QtConcurrent/QtConcurrent>

// Constructor
ArtistService::ArtistService(DiscogsManager* discogs, DatabaseManager* db)
    : m_discogs(discogs), m_db(db) {}

// Public: Search by name
QFuture<std::optional<Artist>> ArtistService::searchArtist(const QString& name) {
    // Run async so UI doesn’t block
    return QtConcurrent::run([=]() -> std::optional<Artist> {
        // First check local DB for matching artist
        for (const auto& artist : m_db->listArtists()) {
            if (artist.name.compare(name, Qt::CaseInsensitive) == 0) {
                return artist; // Found locally
            }
        }

        // Otherwise, query Discogs
        auto discogsResults = m_discogs->search(name).result();
        if (discogsResults.empty()) {
            return std::nullopt; // No match found
        }

        // For now, take first match
        Artist artist = discogsResults.front();

        // Fetch full details from Discogs (releases, profile, etc.)
        auto detailed = m_discogs->fetchArtist(artist.id).result();
        if (!detailed.has_value()) {
            return std::nullopt;
        }

        // Cache result in DB
        cacheArtist(detailed.value());
        return detailed;
    });
}

// Public: Get artist data (by ID)
QFuture<std::optional<Artist>> ArtistService::getArtistData(const QString& artistId) {
    return QtConcurrent::run([=]() -> std::optional<Artist> {
        // Check local DB first
        auto local = m_db->findArtist(artistId);
        if (local.has_value()) {
            return local;
        }

        // Otherwise fetch from Discogs
        auto detailed = m_discogs->fetchArtist(artistId).result();
        if (!detailed.has_value()) {
            return std::nullopt;
        }

        // Save to DB
        cacheArtist(detailed.value());
        return detailed;
    });
}

// Public: list cached artists
std::vector<Artist> ArtistService::listCachedArtists() const {
    return m_db->listArtists();
}

// Private helper: cache artist in DB
void ArtistService::cacheArtist(const Artist& artist) {
    m_db->saveArtist(artist);
}

std::vector<ReleaseInfo> ArtistService::parseReleasesJsonArray(const QJsonArray &releasesArray) {
    std::vector<ReleaseInfo> releases;

    for (const QJsonValue &value : releasesArray) {
        if (!value.isObject()) continue;

        QJsonObject obj = value.toObject();
        ReleaseInfo info;
        info.id = QString::number(obj["id"].toInt());  // or obj["id"].toString() if string
        info.title = obj["title"].toString();
        info.year = obj.contains("year") ? obj["year"].toInt() : 0;
        info.resourceUrl = obj["resource_url"].toString();
        info.role = obj["role"].toString();
        info.artistName = obj["artist"].toString();  // optional, useful for junction table

        releases.push_back(std::move(info));
    }

    return releases;
}

void ArtistService::updateReleasesFromJson(const QJsonArray &jsonReleases, const QString &artistId) {
    auto releases = parseReleasesJsonArray(jsonReleases);
    m_db->saveReleases(artistId, releases);
}
