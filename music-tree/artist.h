#ifndef ARTIST_H
#define ARTIST_H

#pragma once

#include <QString>
#include <ostream>
#include <vector>
#include <QDebug>

struct ReleaseInfo {
    QString id;
    QString title;
    QString artistName;
    int year = 0;
    QString country;
    QString genre;
    QString style;
    QString resourceUrl;
    QString dataQuality;
    QString role; // "Main", "Featuring", etc.
};


struct Artist {
    QString id;
    QString name;
    QString profile;
    QString resourceUrl;
    std::vector<ReleaseInfo> releases;
};


struct CollabKey {
    QString a, b;

    CollabKey(const QString& id1, const QString& id2) {
        if (id1 < id2) { a = id1; b = id2; }
        else           { a = id2; b = id1; }
    }

    bool operator==(const CollabKey& other) const {
        return a == other.a && b == other.b;
    }
};

struct CollabKeyHash {
    std::size_t operator()(const CollabKey& k) const {
        return qHash(k.a) ^ (qHash(k.b) << 1);
    }
};

using ArtistCollaboration = QVector<QString>; // list of shared releases

using SessionCollaborations = std::unordered_map<CollabKey, ArtistCollaboration, CollabKeyHash>;



// Declare operators here
QDebug operator<<(QDebug dbg, const ReleaseInfo &r);
QDebug operator<<(QDebug dbg, const std::vector<ReleaseInfo> &vec);

QDebug operator<<(QDebug dbg, const Artist &a);
QDebug operator<<(QDebug dbg, const std::vector<Artist> &vec);


#endif // ARTIST_H
