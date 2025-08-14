#ifndef DISCOGSMANAGER_H
#define DISCOGSMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class DiscogsManager : public QObject
{
    Q_OBJECT
public:
    explicit DiscogsManager(QObject *parent = nullptr);

    Q_INVOKABLE void fetchArtist(int artistId);

signals:
    void artistDataReady(const QString &json);

private slots:
    void onArtistReply(QNetworkReply *reply);

private:
    QNetworkAccessManager m_netManager;
};

#endif // DISCOGSMANAGER_H
