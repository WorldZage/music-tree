#pragma once

#include <QString>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QDir>
#include <QVariant>
#include <QDebug>
#include <QMap>
#include <QString>
#include <QThread>
#include <QFile>
#include <QStandardPaths>

#include <optional>
#include <vector>



#include "artist.h"


class DatabaseManager {
public:
    DatabaseManager();

    // Thread-safe accessor
    static QSqlDatabase getThreadConnection();

    // Initializes the database schema (tables, indices)
    bool initialize(void);

    // Look up artist by ID
    std::optional<Artist> findArtistById(const QString& artistId) const;

    std::vector<ReleaseInfo> getReleasesForArtist(const QString& artistId) const;

    // Save or update artist in DB
    void saveArtist(const Artist& artist);
    void saveReleases(const QString& artistId, const std::vector<ReleaseInfo>& releases);


    // Delete artist by ID
    void deleteArtist(const QString& artistId);

    // List all stored artists
    std::vector<Artist> listArtists() const;

    // Clear all data
    void clear();




private:
    QString m_dbPath;     // path to SQLite DB file
    QString m_schemaPath; // path to schema.sql in resources

    // Initialization helpers
    bool initializeSchema();

    // TODO: Consider removing:
    std::vector<QString> findCollaborations(const QString& artistId1, const QString& artistId2) const;
    QMap<QString, std::vector<QString>> getAllCollaborations(const QString& artistId) const;
    std::optional<Artist> findArtistByName(const QString& name) const;
};
