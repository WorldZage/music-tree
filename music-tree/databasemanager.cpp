#include "databasemanager.h"


static QString g_dbPath; // global DB path for thread-local connections


DatabaseManager::DatabaseManager() {

    QString dbDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbDir);

    QString dbFileName = "music-tree.db";
    m_dbPath = QDir(dbDir).filePath(dbFileName);
/*
    // Database lives in the app root
    QCoreApplication::applicationFilePath();
    QDir appDir((QCoreApplication::applicationDirPath()));
    QString toAppRoot = "../../";

    QString dbFileName = toAppRoot+"music-tree.db";
    // QString schemaFileName = toAppRoot+"resources/"+"schema.sql";
    m_dbPath = QDir(appDir).filePath(dbFileName);
*/
    m_schemaPath = ":/resources/schema.sql";// appDir.filePath(schemaFileName);

    g_dbPath = m_dbPath; // set global DB path for thread-local connections

    this->initialize();
}

bool DatabaseManager::initialize() {
    static bool initialized = false;
    if (initialized) return true;
    initialized = true;

    QSqlDatabase db = getThreadConnection();
    if (!db.isOpen()) {
        qWarning() << "Database not open at initialization:" << db.lastError().text();
        return false;
    }

    return initializeSchema();
}
bool DatabaseManager::initializeSchema() {
    QSqlDatabase db = getThreadConnection();
    QSqlQuery check(db);
    if (!check.exec("SELECT name FROM sqlite_master WHERE type='table' AND name='artists'")) {
        qWarning() << "Failed to query sqlite_master:" << check.lastError().text();
        return false;
    }

    if (!check.next()) {
        QFile schemaFile(m_schemaPath);

        if (!schemaFile.open(QIODevice::ReadOnly)) {
            qWarning() << "Failed to open schema resource file:" << m_schemaPath;
            return false;
        }
        QString schema = schemaFile.readAll();
        schemaFile.close();

        QStringList statements = schema.split(';', Qt::SkipEmptyParts);
        for (const QString& stmt : std::as_const(statements)) {
            QString trimmed = stmt.trimmed();
            if (trimmed.isEmpty()) continue;

            QSqlQuery query(db);
            if (!query.exec(trimmed)) {
                qWarning() << "Schema creation failed:" << query.lastError().text()
                << "\nStatement:" << trimmed;
                return false;
            }
        }
    }
    return true;
}


QSqlDatabase DatabaseManager::getThreadConnection() {
    thread_local QSqlDatabase db;

    if (!db.isValid()) {
        QString connName = QString("MusicTreeConnection_%1").arg((quintptr)QThread::currentThreadId());
        if (QSqlDatabase::contains(connName)) {
            db = QSqlDatabase::database(connName);
        } else {
            db = QSqlDatabase::addDatabase("QSQLITE", connName);
            db.setDatabaseName(g_dbPath);
            if (!db.open()) {
                qWarning() << "Failed to open DB in thread:" << QThread::currentThreadId()
                           << db.lastError().text();
            }
        }
    }

    return db;
}

// -----------------------------
// Find by ID
// -----------------------------
std::optional<Artist> DatabaseManager::findArtistById(const QString& artistId) const {
    QSqlDatabase db = getThreadConnection();
    QSqlQuery query(db);
    query.prepare("SELECT id, name, profile, resource_url FROM artists WHERE id = :id");
    query.bindValue(":id", artistId);

    if (!query.exec()) {
        qWarning() << "findArtist failed:" << query.lastError().text();
        return std::nullopt;
    }

    if (query.next()) {
        Artist artist;
        artist.id = query.value(0).toString();
        artist.name = query.value(1).toString();
        artist.profile = query.value(2).toString();
        artist.resourceUrl = query.value(3).toString();

        artist.releases = getReleasesForArtist(artist.id); // populate releases

        return artist;
    }
    return std::nullopt;
}

// -----------------------------
// Find by name
// -----------------------------
std::optional<Artist> DatabaseManager::findArtistByName(const QString& name) const {
    QSqlDatabase db = getThreadConnection();
    QSqlQuery query(db);
    query.prepare("SELECT id, name, profile, resource_url FROM artists WHERE name = :name");
    query.bindValue(":name", name);

    if (!query.exec()) {
        qWarning() << "findArtistByName failed:" << query.lastError().text();
        return std::nullopt;
    }

    if (query.next()) {
        Artist artist;
        artist.id = query.value(0).toString();
        artist.name = query.value(1).toString();
        artist.profile = query.value(2).toString();
        artist.resourceUrl = query.value(3).toString();

        artist.releases = getReleasesForArtist(artist.id); // populate releases
        return artist;
    }
    return std::nullopt;
}

// -----------------------------
// Helper: fetch releases for a given artist ID
// -----------------------------
std::vector<ReleaseInfo> DatabaseManager::getReleasesForArtist(const QString& artistId) const {
    std::vector<ReleaseInfo> releases;

    QSqlDatabase db = getThreadConnection();
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT r.id, r.title, r.year, r.country, r.genre, r.style, r.resource_url, r.data_quality, ra.role
        FROM releases r
        JOIN release_artists ra ON r.id = ra.release_id
        WHERE ra.artist_id = :artistId
    )");
    query.bindValue(":artistId", artistId);

    if (!query.exec()) {
        qWarning() << "getReleasesForArtist failed:" << query.lastError().text();
        return releases;
    }

    while (query.next()) {
        ReleaseInfo info;
        info.id = query.value(0).toString();
        info.title = query.value(1).toString();
        info.year = query.value(2).toInt();
        info.country = query.value(3).toString();
        info.genre = query.value(4).toString();
        info.style = query.value(5).toString();
        info.resourceUrl = query.value(6).toString();
        info.dataQuality = query.value(7).toString();
        info.role = query.value(8).toString();
        releases.push_back(info);
    }

    return releases;
}


// -----------------------------
// Insert or update
// -----------------------------
void DatabaseManager::saveArtist(const Artist& artist) {
    QSqlDatabase db = getThreadConnection();

    QSqlQuery update(db);
    update.prepare("UPDATE artists SET name = :name WHERE id = :id");
    update.bindValue(":id", artist.id);
    update.bindValue(":name", artist.name);

    if (!update.exec()) {
        qWarning() << "saveArtist update failed:" << update.lastError().text();
        return;
    }

    if (update.numRowsAffected() == 0) {
        QSqlQuery insert(db);
        insert.prepare("INSERT INTO artists (id, name) VALUES (:id, :name)");
        insert.bindValue(":id", artist.id);
        insert.bindValue(":name", artist.name);

        if (!insert.exec()) {
            qWarning() << "saveArtist insert failed:" << insert.lastError().text();
        }
    }
}


void DatabaseManager::saveReleases(const QString& artistId, const std::vector<ReleaseInfo>& releases) {
    QSqlDatabase db = getThreadConnection();
    QSqlQuery releaseQuery(db);
    QSqlQuery junctionQuery(db);

    releaseQuery.prepare(R"(
        INSERT INTO releases (id, title, year, country, genre, style, resource_url, data_quality)
        VALUES (:id, :title, :year, :country, :genre, :style, :resource_url, :data_quality)
        ON CONFLICT(id) DO UPDATE SET
            title = excluded.title,
            year = excluded.year,
            country = excluded.country,
            genre = excluded.genre,
            style = excluded.style,
            resource_url = excluded.resource_url,
            data_quality = excluded.data_quality
    )");

    junctionQuery.prepare(R"(
        INSERT INTO release_artists (release_id, artist_id, role)
        VALUES (:release_id, :artist_id, :role)
        ON CONFLICT(release_id, artist_id) DO UPDATE SET
            role = excluded.role
    )");

    for (const auto& release : releases) {
        releaseQuery.bindValue(":id", release.id);
        releaseQuery.bindValue(":title", release.title);
        releaseQuery.bindValue(":year", release.year);
        releaseQuery.bindValue(":country", release.country);
        releaseQuery.bindValue(":genre", release.genre);
        releaseQuery.bindValue(":style", release.style);
        releaseQuery.bindValue(":resource_url", release.resourceUrl);
        releaseQuery.bindValue(":data_quality", release.dataQuality);

        if (!releaseQuery.exec()) {
            qWarning() << "Failed to insert/update release:" << releaseQuery.lastError().text()
            << "Release ID:" << release.id;
            continue;
        }

        junctionQuery.bindValue(":release_id", release.id);
        junctionQuery.bindValue(":artist_id", artistId);
        junctionQuery.bindValue(":role", release.role);

        if (!junctionQuery.exec()) {
            qWarning() << "Failed to insert/update release_artists:" << junctionQuery.lastError().text()
            << "Release ID:" << release.id << "Artist ID:" << artistId;
        }
    }
}


// -----------------------------
// Delete
// -----------------------------
void DatabaseManager::deleteArtist(const QString& artistId) {
    QSqlDatabase db = getThreadConnection();
    QSqlQuery query(db);
    query.prepare("DELETE FROM artists WHERE id = :id");
    query.bindValue(":id", artistId);

    if (!query.exec()) {
        qWarning() << "deleteArtist failed:" << query.lastError().text();
    }
}

// -----------------------------
// List all
// -----------------------------
std::vector<Artist> DatabaseManager::listArtists() const {
    std::vector<Artist> artists;
    QSqlDatabase db = getThreadConnection();
    QSqlQuery query(db);

    if (!query.exec("SELECT id, name FROM artists ORDER BY name ASC")) {
        qWarning() << "listArtists failed:" << query.lastError().text();
        return artists;
    }

    while (query.next()) {
        Artist artist;
        artist.id = query.value(0).toString();
        artist.name = query.value(1).toString();
        artists.push_back(artist);
    }
    return artists;
}

// -----------------------------
// Clear DB (wipe all rows, keep schema)
// -----------------------------
void DatabaseManager::clear() {
    QSqlDatabase db = getThreadConnection();
    QSqlQuery query(db);

    // Disable foreign key checks temporarily for full wipe
    if (!query.exec("PRAGMA foreign_keys = OFF;")) {
        qWarning() << "Failed to disable foreign keys:" << query.lastError().text();
        return;
    }

    QStringList tables = {
        "release_labels",
        "labels",
        "tracks",
        "release_artists",
        "releases",
        "members",
        "artists"
    };

    for (const QString &table : tables) {
        if (!query.exec(QString("DELETE FROM %1;").arg(table))) {
            qWarning() << "Failed to clear table" << table << ":" << query.lastError().text();
        }
        // Reset AUTOINCREMENT counters as well
        if (!query.exec(QString("DELETE FROM sqlite_sequence WHERE name='%1';").arg(table))) {
            // Not all tables have AUTOINCREMENT, so ignore errors
        }
    }

    // Re-enable foreign keys
    if (!query.exec("PRAGMA foreign_keys = ON;")) {
        qWarning() << "Failed to re-enable foreign keys:" << query.lastError().text();
    }

    qDebug() << "Database cleared (all rows removed, schema preserved).";
}


// -----------------------------
// Collaborations
// -----------------------------
std::vector<QString> DatabaseManager::findCollaborations(const QString& artistId1, const QString& artistId2) const {
    std::vector<QString> collaborations;
    QSqlDatabase db = getThreadConnection();
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT ra1.release_id
        FROM release_artists ra1
        INNER JOIN release_artists ra2
            ON ra1.release_id = ra2.release_id
        WHERE ra1.artist_id = :artist1
          AND ra2.artist_id = :artist2
    )");
    query.bindValue(":artist1", artistId1);
    query.bindValue(":artist2", artistId2);

    if (!query.exec()) {
        qWarning() << "findCollaborations failed:" << query.lastError().text();
        return collaborations;
    }

    while (query.next()) {
        collaborations.push_back(query.value(0).toString());
    }

    return collaborations;
}

QMap<QString, std::vector<QString>> DatabaseManager::getAllCollaborations(const QString& artistId) const {
    QMap<QString, std::vector<QString>> collaborations;
    QSqlDatabase db = getThreadConnection();
    QSqlQuery query(db);

    query.prepare(R"(
        SELECT ra2.artist_id, ra1.release_id
        FROM release_artists ra1
        JOIN release_artists ra2
          ON ra1.release_id = ra2.release_id
        WHERE ra1.artist_id = :artist
          AND ra2.artist_id != :artist
    )");
    query.bindValue(":artist", artistId);

    if (!query.exec()) {
        qWarning() << "getAllCollaborations failed:" << query.lastError().text();
        return collaborations;
    }

    while (query.next()) {
        QString collaboratorId = query.value(0).toString();
        QString releaseId = query.value(1).toString();
        collaborations[collaboratorId].push_back(releaseId);
    }

    // TODO: Temporary debug print
    qDebug() << "Collaborators for artist" << artistId << ":";

    for (auto it = collaborations.constBegin(); it != collaborations.constEnd(); ++it) {
        QString collaboratorId = it.key();

        // Lookup collaborator name
        QSqlQuery nameQuery(db);
        nameQuery.prepare("SELECT name FROM artists WHERE id = :id");
        nameQuery.bindValue(":id", collaboratorId);

        QString collaboratorName = collaboratorId; // fallback if not found
        if (nameQuery.exec() && nameQuery.next()) {
            collaboratorName = nameQuery.value(0).toString();
        }

        qDebug() << "  -" << collaboratorName << "(" << collaboratorId << ")";
    }

    return collaborations;
}
