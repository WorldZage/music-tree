#include "discogsmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSettings>
#include <QUrlQuery>
#include <QCoreApplication>

DiscogsManager::DiscogsManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_networkManager, &QNetworkAccessManager::finished,
            this, &DiscogsManager::onNetworkReply);

    QString iniPath = QCoreApplication::applicationDirPath() +
                      "/../../../music-tree-config.ini";

    QSettings settings(iniPath, QSettings::IniFormat);

    m_pat_token = settings.value("discogs/token").toString();

}

void DiscogsManager::searchArtistByName(const QString &name)
{

    QString url = "https://api.discogs.com/database/search";
    QUrl qurl(url);
    QUrlQuery query;
    query.addQueryItem("q", name);
    query.addQueryItem("type", "artist");
    query.addQueryItem("per_page", "1");
    qurl.setQuery(query);

    QNetworkRequest request(qurl);
    QString authHeader = "Discogs token=" + m_pat_token;
    request.setRawHeader("Authorization", authHeader.toUtf8());
    request.setRawHeader("User-Agent", app_version);
    auto state = new FetchState{SearchState{ .query = name }};
    QNetworkReply* reply = m_networkManager.get(request);
    reply->setProperty("fetchState", QVariant::fromValue<void*>(state));
}

void DiscogsManager::fetchArtist(QString artistId)
{
    QString url = QString("https://api.discogs.com/artists/%1").arg(artistId);
    QNetworkRequest request(url);
    QString authHeader = "Discogs token=" + m_pat_token;
    request.setRawHeader("Authorization", authHeader.toUtf8());
    request.setRawHeader("User-Agent", app_version);


    auto state = new FetchState{ArtistState{.artistId = artistId}};
    QNetworkReply* reply = m_networkManager.get(request);
    reply->setProperty("fetchState", QVariant::fromValue<void*>(state));
}

void DiscogsManager::fetchReleasesFromUrl(const QString &url, QString artistId, const QString &artistName)
{
    QNetworkRequest request(url + "?per_page=100&page=1");
    QString authHeader = "Discogs token=" + m_pat_token;
    request.setRawHeader("Authorization", authHeader.toUtf8());
    request.setRawHeader("User-Agent", app_version);


    auto state = new FetchState{ReleasesState{.artistId = artistId,
                                              .artistName = artistName,
                                              .url=url,
                                              .page=1}};
    QNetworkReply* reply = m_networkManager.get(request);
    reply->setProperty("fetchState", QVariant::fromValue<void*>(state));
}

void DiscogsManager::onNetworkReply(QNetworkReply *reply)
{
    FetchState* state = static_cast<FetchState*>(reply->property("fetchState").value<void*>());

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network error:" << reply->errorString();
        delete state;
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);


    std::visit([&](auto&& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, SearchState>) {
            onSearchResultReceived(jsonDoc, s);
        } else if constexpr (std::is_same_v<T, ArtistState>) {
            onArtistDataReceived(jsonDoc, s);
        } else if constexpr (std::is_same_v<T, ReleasesState>) {
            onReleasesDataReceived(jsonDoc, s);
        }
    }, *state);

    delete state;
    reply->deleteLater();
}

QString DiscogsManager::extractId(QJsonValue idValue) {
    QString artistId;
    if (idValue.isString()) {
        artistId = idValue.toString();
    } else if (idValue.isDouble()) {  // JSON numbers come in as double in Qt
        artistId = QString::number(static_cast<qint64>(idValue.toDouble()));
    } else {
        qWarning() << "Unexpected ID type:" << idValue;
    }

    return artistId;
}

void DiscogsManager::onSearchResultReceived(const QJsonDocument &jsonDoc, const SearchState searchState)
{
    if (!jsonDoc.isObject()) {
        qWarning() << "Invalid JSON received for search";
        return;
    }

    QJsonArray results = jsonDoc.object()["results"].toArray();
    if (results.isEmpty()) {
        qWarning() << "No artist found";
        return;
    }

    QJsonObject artistObj = results.first().toObject();
    QJsonValue idValue = artistObj["id"];
    QString artistId = extractId(idValue);


    QString name = artistObj["title"].toString();

    qDebug() << "Found artist:" << name << "(" << artistId << ")";
    fetchArtist(artistId);
}


void DiscogsManager::onArtistDataReceived(const QJsonDocument &jsonDoc, ArtistState artistState)
{
    if (!jsonDoc.isObject()) {
        qWarning() << "Invalid JSON received for artist";
        return;
    }

    QJsonObject artistObj = jsonDoc.object();
    QString name = artistObj["name"].toString();
    QString id = extractId(artistObj["id"]);
    QString releasesUrl = artistObj["releases_url"].toString();

    if (!releasesUrl.isEmpty()) {
        fetchReleasesFromUrl(releasesUrl, id, name);
    }
}

void DiscogsManager::onReleasesDataReceived(const QJsonDocument &jsonDoc, ReleasesState releasesState)
{
    qDebug() << "onreleasedata";
    if (!jsonDoc.isObject()) {
        qWarning() << "Invalid JSON received for releases";
        return;
    }

    QJsonObject obj = jsonDoc.object();
    QJsonArray releases = obj["releases"].toArray();

    // Build or merge artist data
    auto it = std::find_if(artistSet.begin(), artistSet.end(),
                           [&](const ArtistData &a) { return a.id == releasesState.artistId; });

    ArtistData *artistPtr = nullptr;
    if (it == artistSet.end()) {
        artistSet.append(ArtistData{.id = releasesState.artistId, .name = releasesState.artistName, .releasesById = {}});
        artistPtr = &artistSet.back();
    } else {
        artistPtr = &(*it);
    }

    // Insert releases
    for (const QJsonValue &releaseValue : std::as_const(releases)) {
        QJsonObject releaseObj = releaseValue.toObject();
        if (releaseObj["type"].toString() == "master") {
            QString releaseId = extractId(releaseObj["id"]);
            artistPtr->releasesById.insert(releaseId, releaseObj["title"].toString());
            qDebug() << "release name:" << releaseObj["title"].toString();
        }
    }



    // Check pagination
    QJsonObject pagination = obj["pagination"].toObject();
    int page = pagination["page"].toInt();
    int pages = pagination["pages"].toInt();
    const int maxPages = 10;

    qDebug() << "artist: " << artistPtr->name <<". page: " << page;
    if ((page < pages) && page < maxPages) {
        // Fetch next page
        int nextPage = page + 1;
        QString nextUrl = QString("%1?per_page=100&page=%2")
                              .arg(releasesState.url)
                              .arg(nextPage);

        auto nextState = new FetchState{ReleasesState{.artistId = releasesState.artistId,
                                                      .artistName = releasesState.artistName,
                                                      .url = releasesState.url,
                                                      .page = nextPage}};
        //releasesState;
        //nextState.page = nextPage;

        QNetworkRequest request(nextUrl);
        QString authHeader = "Discogs token=" + m_pat_token;
        request.setRawHeader("Authorization", authHeader.toUtf8());
        request.setRawHeader("User-Agent", app_version);

        QNetworkReply *reply = m_networkManager.get(request);
        // Store the state for this reply
        reply->setProperty("fetchState", QVariant::fromValue<void*>(nextState));
    }
    else {
        generateCollaborations(*artistPtr);
    }

    //qDebug() << "number of releases: " << releases.size();
    //qDebug() << "name" << artistPtr->name << ". id: " << artistPtr->id ;
}

void DiscogsManager::removeArtist(const QString &artistId)
{
    // Remove from artistSet
    artistSet.erase(std::remove_if(artistSet.begin(), artistSet.end(),
                                   [&](const ArtistData &a) { return a.id == artistId; }), artistSet.end());

    // Remove overlaps containing this artist
    for (auto it = overlaps.begin(); it != overlaps.end();) {
        if (it.key().first == artistId || it.key().second == artistId) {
            it = overlaps.erase(it);
        } else {
            ++it;
        }
    }
}

static QPair<QString, QString> orderedPair(const QString &a, const QString &b) {
    return a < b ? qMakePair(a, b) : qMakePair(b, a);
}

void DiscogsManager::generateCollaborations(const ArtistData &newArtist)
{
    qDebug() << "checkforoverlaps" ;
    QStringList collaborators;

    for (const ArtistData &other : std::as_const(artistSet)) {
        if (other.id == newArtist.id)
            continue;

        QList<QPair<QString, QString>> shared;
        for (auto it = newArtist.releasesById.constBegin();
             it != newArtist.releasesById.constEnd(); ++it) {
            if (other.releasesById.contains(it.key())) {
                shared.append(qMakePair(it.key(), it.value())); // (releaseId, releaseName)
            }
        }

        if (!shared.isEmpty()) {
            auto key = orderedPair(newArtist.id, other.id);
            overlaps.insert(key, shared);

            qDebug() << "Overlap between" << newArtist.name << "and" << other.name << ":";
            for (const auto &rel : std::as_const(shared)) {
                qDebug() << "   Release:" << rel.second << "(" << rel.first << ")";
            }

            collaborators.append(other.name);
        }
    }

    // Always emit one signal per new artist
    emit artistAdded(newArtist.name, collaborators);
}

void DiscogsManager::filterMastersAndLog(const QJsonArray &releases )
{

    QJsonArray masterReleases;
    for (const QJsonValue &val : std::as_const(releases)) {
        QJsonObject releaseObj = val.toObject();
        if (releaseObj["type"].toString() == "master") {
            masterReleases.append(releaseObj);
            qDebug() << "Master Release:" << releaseObj["title"].toString() <<
                "( " << releaseObj["id"] << ")";
        }
    }

    qDebug() << "Total master releases:" << masterReleases.size();
}

