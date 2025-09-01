#include "artistservice.h"
#include "discogsmanager.h"
#include "databasemanager.h"

#include <QtConcurrent/QtConcurrent>

// Constructor
ArtistService::ArtistService(QObject* parent) : QObject(parent){

    // Connect DiscogsManager signals to ArtistService slots
    connect(&m_discogs, &DiscogsManager::discogsArtistSearchReady,
            this, &ArtistService::onDiscogsArtistSearchReady);

    connect(&m_discogs, &DiscogsManager::discogsArtistDataReady,
            this, &ArtistService::onDiscogsDataReady);

    connect(this, &ArtistService::artistFound,
            this, &ArtistService::onArtistFound);

}

void ArtistService::clearDb(void) {
    m_db.clear();
}

// Public: Search by name
void ArtistService::searchByName(const QString& name) {
    qDebug() << "searching for: " << name;
    // 1. Check local DB
    m_discogs.searchForArtistByName(name);
}


void ArtistService::onDiscogsArtistSearchReady(const std::vector<Artist>& artists) {

    qDebug() << "artists found by discog search: " << artists;

    if (artists.empty()) {
        return;
    }
    // TODO: prompt UI selection of top artists found, displaying artist name and artist id.
    // for now, choose top result.
    Artist artist = artists.front();
    // TODO: use the ID from discogs search, to check DB again.
    auto artistOpt = m_db.findArtistById(artist.id);
    if (artistOpt.has_value()) {
        qDebug() << "Found artist by name in DB: " << artist.name;
        Artist db_artist = artistOpt.value();
        emit artistFound(db_artist);
        return;
    }
    else {
        // 2. Not in DB → fetch from Discogs
        m_discogs.fetchArtist(artist.id);
        return;
    }
}

// Called when DiscogsManager has fetched artist & release info
void ArtistService::onDiscogsDataReady(const Artist& artist) {
    // Cache artist & releases asynchronously to DB
    cacheArtist(artist);

    // Emit signals as if artist was found in DB
    emit artistFound(artist);
}

void ArtistService::onArtistFound(const Artist& artist) {
    m_session.addArtist(artist);
    // Find collaborations (TODO: with currently displayed artists)

  ; // placeholder
    /* for context, this is the data type of m_sesison.artists:
     *  struct SessionArtist {
    QString id;    // Discogs ID
    QString name;  // Artist name
};
using SessionArtists = QVector<SessionArtist>;


Keep in mind, it's different from the Artist data type:
struct Artist {
    QString id;
    QString name;
    QString profile;
    QString resourceUrl;
    std::vector<ReleaseInfo> releases;
};
// Which is something we should likely redesign (later), since its confusing.
     *
     */

}
/*
    //QMap<QString, std::vector<QString>> collabs;
    //collabs = m_db.getAllCollaborations(artist.id);
    /*
    for (const auto& otherId : m_currentUIArtistIds) {
        auto rels = m_db.findCollaborations(artist.id, otherId);
        if (!rels.empty()) collabs[otherId] = rels;
    }

emit collaborationsReady(collabs);
qDebug() << "collabs:" << collabs;
 */




// Public: list cached artists
std::vector<Artist> ArtistService::listCachedArtists() const {
    return m_db.listArtists();
}

// Private helper: cache artist in DB
void ArtistService::cacheArtist(const Artist& artist) {
    qDebug() << "Storing Artist: " << artist;
    //QtConcurrent::run([=]() {
        m_db.saveArtist(artist);
        m_db.saveReleases(artist.id, artist.releases);
    //});
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
    m_db.saveReleases(artistId, releases);
}
