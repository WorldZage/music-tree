#ifndef ARTIST_H
#define ARTIST_H

#pragma once

#include <QString>

#include <vector>


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


#endif // ARTIST_H
