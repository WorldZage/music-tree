#include "discogsmanager.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QDebug>

static const QString TOKEN = "YOUR_DISCOGS_TOKEN_HERE";

DiscogsManager::DiscogsManager(QObject *parent)
    : QObject(parent)
{
}

void DiscogsManager::fetchArtist(int artistId)
{
    QUrl url(QString("https://api.discogs.com/artists/%1").arg(artistId));
    QNetworkRequest request(url);

    // Add headers
    request.setRawHeader("Authorization", QByteArray("Discogs token=") + TOKEN.toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader, "MusicTreeApp/1.0");

    QNetworkReply *reply = m_netManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onArtistReply(reply);
    });
}

void DiscogsManager::onArtistReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Request failed:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    emit artistDataReady(QString::fromUtf8(data));
    reply->deleteLater();
}
