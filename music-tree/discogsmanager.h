#ifndef DISCOGSMANAGER_H
#define DISCOGSMANAGER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include "artist.h"

struct ArtistData {
    QString id;
    QString name;
    QHash<QString, QString> releasesById;
};

struct SearchState {
    QString query;
};

struct ArtistState {
    QString artistId;
};

struct ReleasesState {
    QString artistId;
    QString artistName;
    QString url;
    int page = 1;
};

using FetchState = std::variant<SearchState, ArtistState, ReleasesState>;

class DiscogsManager : public QObject
{
    Q_OBJECT

public:
    explicit DiscogsManager(QObject *parent = nullptr);

    // Search Discogs by name, return matching artist IDs
    QFuture<std::vector<Artist>> search(const QString& name);

    // Fetch complete artist details (profile, releases, etc.)
    QFuture<std::optional<Artist>> fetchArtist(const QString& artistId);

    QFuture<std::vector<ReleaseInfo>> fetchAllReleases(const QString& url);

private:
    void fetchReleasesPage(const QString& pageUrl, QSharedPointer<std::vector<ReleaseInfo>> accumulator, QPromise<std::vector<ReleaseInfo>> promise);

    QNetworkAccessManager m_networkManager;

    // Discogs API token. Initialized in the constructor.
    QString m_pat_token ;
    QByteArray app_version = "QtDiscogsApp/1.0";


};



#endif // DISCOGSMANAGER_H
