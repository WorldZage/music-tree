#ifndef DISCOGSMANAGER_H
#define DISCOGSMANAGER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QSettings>
#include <QUrlQuery>
#include <QCoreApplication>

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

    void searchForArtistByName(const QString& name); // main-thread network call
    void fetchArtist(const QString& artistId);

signals:
    void discogsArtistSearchReady(const std::vector<Artist>& artistIds);
    void discogsArtistDataReady(const Artist& artist);



private:
    // Search Discogs by name, return matching artist IDs
    QFuture<std::vector<Artist>> _helper_search(const QString& name);

    // Fetch complete artist details (profile, releases, etc.)
    QFuture<std::optional<Artist>> _helper_fetchArtist(const QString& artistId);

    QFuture<std::vector<ReleaseInfo>> _helper_fetchAllReleases(const QString& url);


    void _helper_fetchReleasesPage(const QString& baseUrl,
                                           int page,
                                           QSharedPointer<std::vector<ReleaseInfo>> accumulator,
                           QPromise<std::vector<ReleaseInfo>> promise);

    QNetworkAccessManager m_networkManager;

    // Discogs API token. Initialized in the constructor.
    QString m_pat_token ;
    QByteArray app_version = "music-tree-app/1.0";
    const int maxPages = 4;


};



#endif // DISCOGSMANAGER_H
