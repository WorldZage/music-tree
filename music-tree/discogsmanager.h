#ifndef DISCOGSMANAGER_H
#define DISCOGSMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>

struct ArtistData {
    QString name;
    QString id;
    QHash<QString, QString> releasesById;
};

class DiscogsManager : public QObject
{
    Q_OBJECT

public:
    explicit DiscogsManager(QObject *parent = nullptr);

    // Entry point from QML
    Q_INVOKABLE void fetchArtist(QString artistId);
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
    void fetchReleasesFromUrl(const QString &url, QString artistId, const QString &artistName); // updated
    void onArtistDataReceived(const QJsonDocument &jsonDoc);
    void onReleasesDataReceived(const QJsonDocument &jsonDoc);

    void onSearchResultReceived(const QJsonDocument &jsonDoc);
    void checkForOverlaps(const ArtistData &newArtist);

    void removeArtist(const QString &artistId);
    QString extractId(QJsonValue idValue);

    // Track what we are waiting for
    enum class RequestType { None, Search, Artist, Releases };
    RequestType m_pendingRequestType = RequestType::None;

    enum class SearchType { Release, Master, Artist, Label};


    QVector<ArtistData> artistSet;
    QHash<QPair<QString, QString>, QList<QPair<QString, QString>>> overlaps;
    // key: (artistId1, artistId2)
    // value: list of (releaseId, releaseName)


    QNetworkAccessManager m_networkManager;

    // Discogs API token. Initialized in the constructor.
    QString m_pat_token ;
    QByteArray app_version = "QtDiscogsApp/1.0";

    // store temp artist info between API calls
    QString m_currentArtistId = "";
    QString m_currentArtistName;

};



#endif // DISCOGSMANAGER_H
