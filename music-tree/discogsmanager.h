#ifndef DISCOGSMANAGER_H
#define DISCOGSMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>

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

    // Entry point from QML
    Q_INVOKABLE void searchArtistByName(const QString &name);


signals:
    void artistDataReady(const QString &json);
    void releasesDataReady(const QString &json);
    void artistAdded(const QString &artistName, const QStringList &collaborators);


private slots:
    // Single dispatcher for all finished replies
    void onNetworkReply(QNetworkReply *reply);
    void filterMastersAndLog(const QJsonArray &releases );

private:
    // Internal helpers
    void onSearchResultReceived(const QJsonDocument &jsonDoc, SearchState searchState);
    void onArtistDataReceived(const QJsonDocument &jsonDoc, ArtistState artistState);
    void onReleasesDataReceived(const QJsonDocument &jsonDoc, ReleasesState releasesState);

    void fetchArtist(QString artistId);
    void fetchReleasesFromUrl(const QString &url, QString artistId, const QString &artistName); // updated

    void generateCollaborations(const ArtistData &newArtist);

    void removeArtist(const QString &artistId);
    QString extractId(QJsonValue idValue);

    QVector<ArtistData> artistSet;
    QHash<QPair<QString, QString>, QList<QPair<QString, QString>>> overlaps;
    // key: (artistId1, artistId2)
    // value: list of (releaseId, releaseName)


    QNetworkAccessManager m_networkManager;

    // Discogs API token. Initialized in the constructor.
    QString m_pat_token ;
    QByteArray app_version = "QtDiscogsApp/1.0";

};



#endif // DISCOGSMANAGER_H
