// SessionManager.h
#pragma once
#include <QObject>
#include <QString>
#include <QVector>

struct SessionArtist {
    QString id;    // Discogs ID
    QString name;  // Artist name
};

class SessionManager : public QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject* parent = nullptr);

    const QVector<SessionArtist>& artists() const { return m_artists; }

    Q_INVOKABLE void addArtist(const SessionArtist& artist);
    Q_INVOKABLE void addArtist(const QString& artistName, const QString& artistId);
    Q_INVOKABLE void removeArtist(const QString& artistId);
    void clear();

signals:
    void artistAdded(const SessionArtist& artist);
    void artistRemoved(const QString& artistId);
    void sessionCleared();

private:
    QVector<SessionArtist> m_artists;
};
