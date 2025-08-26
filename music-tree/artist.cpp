#include "artist.h"

QDebug operator<<(QDebug dbg, const ReleaseInfo &r)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "ReleaseInfo(" << r.title << ", " << r.year << ")";
    return dbg;
}

QDebug operator<<(QDebug dbg, const std::vector<ReleaseInfo> &vec)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        dbg << vec[i];
        if (i + 1 < vec.size())
            dbg << ", ";
    }
    dbg << "]";
    return dbg;
}


QDebug operator<<(QDebug dbg, const Artist &a)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "Artist(" << a.name << ", " << a.id <<")";
    return dbg;
}

QDebug operator<<(QDebug dbg, const std::vector<Artist> &vec)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        dbg << vec[i];
        if (i + 1 < vec.size())
            dbg << ", ";
    }
    dbg << "]";
    return dbg;
}
