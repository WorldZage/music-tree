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
    qDebug() << "ini path: " + iniPath;

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

    m_pendingRequestType = RequestType::Search;
    m_networkManager.get(request);
}

void DiscogsManager::fetchArtist(QString artistId)
{
    QString url = QString("https://api.discogs.com/artists/%1").arg(artistId);
    QNetworkRequest request(url);
    QString authHeader = "Discogs token=" + m_pat_token;
    request.setRawHeader("Authorization", authHeader.toUtf8());
    request.setRawHeader("User-Agent", app_version);

    m_pendingRequestType = RequestType::Artist;
    m_networkManager.get(request);
}

void DiscogsManager::fetchReleasesFromUrl(const QString &url, QString artistId, const QString &artistName)
{
    QNetworkRequest request(url + "?per_page=100&page=1");
    QString authHeader = "Discogs token=" + m_pat_token;
    request.setRawHeader("Authorization", authHeader.toUtf8());
    request.setRawHeader("User-Agent", app_version);

    m_currentArtistId = artistId;
    m_currentArtistName = artistName;
    m_pendingRequestType = RequestType::Releases;
    m_networkManager.get(request);
}

void DiscogsManager::onNetworkReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Network error:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

    switch (m_pendingRequestType) {
    case RequestType::Artist:   onArtistDataReceived(jsonDoc); break;
    case RequestType::Releases: onReleasesDataReceived(jsonDoc); break;
    case RequestType::Search:   onSearchResultReceived(jsonDoc); break;
    default: break;
    }

    reply->deleteLater();
}

void DiscogsManager::onSearchResultReceived(const QJsonDocument &jsonDoc)
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
    QString artistId;

    if (idValue.isString()) {
        artistId = idValue.toString();
    } else if (idValue.isDouble()) {  // JSON numbers come in as double in Qt
        artistId = QString::number(static_cast<qint64>(idValue.toDouble()));
    } else {
        qWarning() << "Unexpected ID type:" << idValue;
    }

    QString name = artistObj["title"].toString();

    qDebug() << "Found artist:" << name << "(" << artistId << ")";
    fetchArtist(artistId);
}


void DiscogsManager::onArtistDataReceived(const QJsonDocument &jsonDoc)
{
    if (!jsonDoc.isObject()) {
        qWarning() << "Invalid JSON received for artist";
        return;
    }

    QJsonObject artistObj = jsonDoc.object();
    QString name = artistObj["name"].toString();
    QString id = artistObj["id"].toString();
    QString releasesUrl = artistObj["releases_url"].toString();

    if (!releasesUrl.isEmpty()) {
        fetchReleasesFromUrl(releasesUrl, id, name);
    }
}

void DiscogsManager::onReleasesDataReceived(const QJsonDocument &jsonDoc)
{
    qDebug() << "onreleasedata";
    if (!jsonDoc.isObject()) {
        qWarning() << "Invalid JSON received for releases";
        return;
    }

    QJsonObject obj = jsonDoc.object();
    QJsonArray releases = obj["releases"].toArray();

    // Build new artist data
    ArtistData newArtist;
    newArtist.id = m_currentArtistId;
    newArtist.name = m_currentArtistName;

    for (const QJsonValue &releaseValue : std::as_const(releases)) {
        QJsonObject releaseObj = releaseValue.toObject();
        QString type = releaseObj["type"].toString();
        if (type == "master") {
            newArtist.releaseIds.insert(releaseObj["id"].toVariant().toString());
        }
    }

    artistSet.append(newArtist);
    checkForOverlaps(newArtist);

    filterMastersAndLog(releases);
}

void DiscogsManager::checkForOverlaps(const ArtistData &newArtist)
{
    qDebug() << "checkforoverlaps";

    for (const ArtistData &other : std::as_const(artistSet)) {
        if (other.id == newArtist.id) { continue; }
        qDebug() << other.name;
        for (const QString &id : std::as_const(other.releaseIds)) {
            qDebug() << "release id " << id;
        }


        QSet<QString> overlap = other.releaseIds & newArtist.releaseIds;
        if (!overlap.isEmpty()) {
            qDebug() << "Overlap between" << newArtist.name << "and" << other.name << ":";
            for (const QString &id : std::as_const(overlap)) {
                qDebug() << "   Release ID:" << id;
            }
        }
    }
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

