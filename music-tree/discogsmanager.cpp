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
    //connect(&m_networkManager, &QNetworkAccessManager::finished,
    //        this, &DiscogsManager::onNetworkReply);


    QString iniPath = QCoreApplication::applicationDirPath() +
                      "/../../../music-tree-config.ini";
    QSettings settings(iniPath, QSettings::IniFormat);
    m_pat_token = settings.value("discogs/token").toString();

}

// Search artist by name and return a future of vector<Artist> (take first match if desired)
QFuture<std::vector<Artist>> DiscogsManager::search(const QString& name)
{
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
QFuture<std::optional<Artist>> DiscogsManager::fetchArtist(const QString& artistId)
{
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
            fetchAllReleases(releasesUrl).then([p = std::move(p), artist](std::vector<ReleaseInfo> releases) mutable {
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
void DiscogsManager::fetchReleasesPage(const QString& pageUrl,
                                       QSharedPointer<std::vector<ReleaseInfo>> accumulator,
                                       QPromise<std::vector<ReleaseInfo>> promise)
{
    QNetworkRequest request(pageUrl);
    request.setRawHeader("Authorization", QString("Discogs token=%1").arg(m_pat_token).toUtf8());
    request.setRawHeader("User-Agent", app_version);

    QNetworkReply* reply = m_networkManager.get(request);
    QObject::connect(reply, &QNetworkReply::finished, [this, reply, accumulator, p = std::move(promise), pageUrl]() mutable {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
            QJsonArray releases = obj["releases"].toArray();

            for (const auto& val : releases) {
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
            int page = pagination["page"].toInt();
            int pages = pagination["pages"].toInt();

            if (page < pages) {
                QString nextUrl = QString("%1?per_page=100&page=%2").arg(pageUrl).arg(page + 1);
                fetchReleasesPage(nextUrl, accumulator, std::move(p));
                reply->deleteLater();
                return;
            }
        } else {
            qWarning() << "Fetch releases error:" << reply->errorString();
        }

        // Finished all pages
        reply->deleteLater();
        p.addResult(*accumulator);
        p.finish();
    });
}

// Public function
QFuture<std::vector<ReleaseInfo>> DiscogsManager::fetchAllReleases(const QString& url)
{
    QPromise<std::vector<ReleaseInfo>> promise;
    auto future = promise.future();
    auto accumulator = QSharedPointer<std::vector<ReleaseInfo>>::create();
    fetchReleasesPage(url, accumulator, std::move(promise));
    return future;
}


