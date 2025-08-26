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

// Declare operators here
QDebug operator<<(QDebug dbg, const ReleaseInfo &r);
QDebug operator<<(QDebug dbg, const std::vector<ReleaseInfo> &vec);

QDebug operator<<(QDebug dbg, const Artist &a);
QDebug operator<<(QDebug dbg, const std::vector<Artist> &vec);


#endif // ARTIST_H
