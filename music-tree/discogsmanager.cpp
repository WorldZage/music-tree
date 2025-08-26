#include "discogsmanager.h"


DiscogsManager::DiscogsManager(QObject *parent)
    : QObject(parent)
{
    //connect(&m_networkManager, &QNetworkAccessManager::finished,
    //        this, &DiscogsManager::onNetworkReply);


    QString iniPath = QCoreApplication::applicationDirPath() +
                      "/../../../music-tree-config.ini";
    QSettings settings(iniPath, QSettings::IniFormat);
    m_pat_token = settings.value("discogs/token").toString();

}



void DiscogsManager::searchForArtistByName(const QString& name)
{
    qDebug() << "discog search begun with: " << name;
    // Kick off the async _helper_search (this returns immediately)
    QFuture<std::vector<Artist>> fSearch = _helper_search(name);

    // Watcher for the search future
    auto *searchWatcher = new QFutureWatcher<std::vector<Artist>>(this);

    // When the search completes (signal emitted in this object's thread)
    QObject::connect(searchWatcher, &QFutureWatcherBase::finished,
                     this, [this, searchWatcher, name]() {
                         // Take the results
                         const std::vector<Artist> artists = searchWatcher->result();
                         searchWatcher->deleteLater();

                         if (artists.empty()) {
                             qWarning() << "No artists found for" << name;
                         }
                         emit discogsArtistSearchReady(artists);

    });
    // Start watching the search future
    searchWatcher->setFuture(fSearch);
    return;
}

void DiscogsManager::fetchArtist(const QString& artistId) {
    QFuture<std::optional<Artist>> fDetail = _helper_fetchArtist(artistId);

    // Watcher for the detail future
    auto *detailWatcher = new QFutureWatcher<std::optional<Artist>>(this);
    QObject::connect(detailWatcher, &QFutureWatcherBase::finished,
                     this, [this, detailWatcher]() {
                         const std::optional<Artist> opt = detailWatcher->result();
                         detailWatcher->deleteLater();

                         if (!opt.has_value()) {
                             qWarning() << "Failed to fetch detailed artist info";
                             return;
                         }

                         const Artist full = *opt; // copy ok
                         qDebug() << "artist: " << full.name << ". releases: " << full.releases;

                         emit discogsArtistDataReady(full);
                     });

    // Start watching the detail future
    detailWatcher->setFuture(fDetail);
    return;
}

// Search artist by name and return a future of vector<Artist> (take first match if desired)
QFuture<std::vector<Artist>> DiscogsManager::_helper_search(const QString& name)
{
    qDebug() << "discog search fnc begun with: " << name;
    QPromise<std::vector<Artist>> promise;
    auto future = promise.future();

    QString url = "https://api.discogs.com/database/search";
    QUrl qurl(url);
    QUrlQuery query;
    query.addQueryItem("q", name);
    query.addQueryItem("type", "artist");
    query.addQueryItem("per_page", "5"); // can adjust
    qurl.setQuery(query);

    QNetworkRequest request(qurl);;
    request.setRawHeader("Authorization", QString("Discogs token=%1").arg(m_pat_token).toUtf8());
    request.setRawHeader("User-Agent", app_version);

    QNetworkReply* reply = m_networkManager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, [reply, p = std::move(promise)]() mutable {
        std::vector<Artist> result;
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray results = doc.object()["results"].toArray();
            for (const auto& val : results) {
                QJsonObject obj = val.toObject();
                QString id = obj["id"].isString() ? obj["id"].toString()
                                                  : QString::number(static_cast<qint64>(obj["id"].toDouble()));
                result.push_back(Artist{.id = id, .name = obj["title"].toString()});
            }
        } else {
            qWarning() << "Discogs search error:" << reply->errorString();
        }
        reply->deleteLater();
        p.addResult(result);
        p.finish();
    });

    return future;
}

// Fetch detailed artist data including releases
QFuture<std::optional<Artist>> DiscogsManager::_helper_fetchArtist(const QString& artistId)
{
    qDebug() << "discog fetchArtist fnc begun with: " << artistId;
    QPromise<std::optional<Artist>> promise;
    auto future = promise.future();

    QString url = QString("https://api.discogs.com/artists/%1").arg(artistId);
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Discogs token=%1").arg(m_pat_token).toUtf8());
    request.setRawHeader("User-Agent", app_version);

    QNetworkReply* reply = m_networkManager.get(request);

    QObject::connect(reply, &QNetworkReply::finished, [reply, artistId, p = std::move(promise), this]() mutable {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Fetch artist error:" << reply->errorString();
            reply->deleteLater();
            p.addResult(std::nullopt);
            p.finish();
            return;
        }

        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        Artist artist;
        artist.id = QString::number(obj["id"].toDouble());
        artist.name = obj["name"].toString();
        artist.profile = obj["profile"].toString();
        artist.resourceUrl = obj["resource_url"].toString();

        // If you want releases, fetch them next
        QString releasesUrl = obj["releases_url"].toString();
        if (!releasesUrl.isEmpty()) {
            _helper_fetchAllReleases(releasesUrl).then([p = std::move(p), artist](std::vector<ReleaseInfo> releases) mutable {
                artist.releases = releases;
                p.addResult(artist);
                p.finish();
            });
        } else {
            p.addResult(artist);
            p.finish();
        }

        reply->deleteLater();
    });

    return future;
}

// Member helper
void DiscogsManager::_helper_fetchReleasesPage(const QString& baseUrl,
                                       int page,
                                       QSharedPointer<std::vector<ReleaseInfo>> accumulator,
                                       QPromise<std::vector<ReleaseInfo>> promise)
{
    QString url = QString("%1?per_page=100&page=%2").arg(baseUrl).arg(page);
    qDebug() << "discog fetchrelease fnc begun. time: " <<QDateTime::currentSecsSinceEpoch() << ". pageurl: " << url;

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Discogs token=%1").arg(m_pat_token).toUtf8());
    request.setRawHeader("User-Agent", app_version);

    QNetworkReply* reply = m_networkManager.get(request);
    QObject::connect(reply, &QNetworkReply::finished,
                     [this, reply, baseUrl, page, accumulator, p = std::move(promise)]() mutable {

                         if (reply->error() == QNetworkReply::NoError) {
                             QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
                             QJsonArray releases = obj["releases"].toArray();

                             for (const auto& val : std::as_const(releases)) {
                                 QJsonObject r = val.toObject();
                                 if (r["type"].toString() == "master") {
                                     ReleaseInfo info;
                                     info.id = QString::number(r["id"].toDouble());
                                     info.title = r["title"].toString();
                                     info.year = r["year"].toInt();
                                     info.resourceUrl = r["resource_url"].toString();
                                     info.role = r["role"].toString();
                                     accumulator->push_back(info);
                                 }
                             }
                             // Pagination
                             QJsonObject pagination = obj["pagination"].toObject();
                             int totalPages = pagination["pages"].toInt();
                             qDebug() << "total pages: " << totalPages;
                             if (page < std::min(totalPages, maxPages)) {
                                 _helper_fetchReleasesPage(baseUrl, page + 1, accumulator, std::move(p));
                                 reply->deleteLater();
                                 return;
                             }
                         } else {
                             qWarning() << "Fetch releases error:" << reply->errorString();
                         }

                         // Finished
                         reply->deleteLater();
                         p.addResult(*accumulator);
                         p.finish();
                     });
}

QFuture<std::vector<ReleaseInfo>> DiscogsManager::_helper_fetchAllReleases(const QString& url)
{
    qDebug() << "discog fetchallreleases fnc";
    QPromise<std::vector<ReleaseInfo>> promise;
    auto future = promise.future();
    auto accumulator = QSharedPointer<std::vector<ReleaseInfo>>::create();
    _helper_fetchReleasesPage(url, 1, accumulator, std::move(promise));
    return future;
}


