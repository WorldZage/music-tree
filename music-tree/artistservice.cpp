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
    // Cache artist & releases to DB
    cacheArtist(artist);

    // Emit signals as if artist was found in DB
    emit artistFound(artist);
}

void ArtistService::onArtistFound(const Artist& artist) {
    m_session.addArtist(artist);
}


// Public: list cached artists
std::vector<Artist> ArtistService::listCachedArtists() const {
    return m_db.listArtists();
}

// Private helper: cache artist in DB
void ArtistService::cacheArtist(const Artist& artist) {
    qDebug() << "Storing Artist: " << artist;
    m_db.saveArtist(artist);
    m_db.saveReleases(artist.id, artist.releases);
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

void ArtistService::loadArtistsFromFile() {
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    tr("Open File"),
                                                    "/home",
                                                    tr("JSON files (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open file for reading:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid JSON format";
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray artistArray = root["artists"].toArray();

    for (const QJsonValue &value : std::as_const(artistArray)) {
        if (!value.isObject()) continue;

        QJsonObject artistObj = value.toObject();
        // QString id = artistObj["id"].toString();
        QString name = artistObj["name"].toString();

        if (!name.isEmpty()) {
            searchByName(name);
            // optionally you could check if ID already exists in local DB,
            // or fetch by ID if that's more reliable than by name
        }
    }
}
void ArtistService::saveArtistsToFile() {
    QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                    tr("Save File"),
                                                    "/home/artists.json",
                                                    tr("JSON files (*.json)"));
    if (fileName.isEmpty()) {
        return;
    }

    QJsonArray artistArray;
    for (const auto &artist : sessionArtists()) {
        QJsonObject artistObj;
        artistObj["id"] = artist.id;       // assuming you have id and name fields
        artistObj["name"] = artist.name;
        artistArray.append(artistObj);
    }

    QJsonObject root;
    root["artists"] = artistArray;

    QJsonDocument doc(root);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open file for writing:" << file.errorString();
        return;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}
