// SessionArtistModel.h
#pragma once
#include <QAbstractListModel>

#include <QMetaEnum>
#include "sessionmanager.h"
#include "artistservice.h"


class SessionArtistModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit SessionArtistModel(ArtistService* artistService, QObject* parent = nullptr);

    //enum Roles { IdRole = Qt::UserRole + 1, NameRole };
    enum Roles { ArtistIdRole = Qt::UserRole + 1, ArtistNameRole};
    Q_ENUM(Roles);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    //QHash<int, QByteArray> roleNames() const override;
    QHash<int, QByteArray> roleNames() const override {
        return { {ArtistIdRole, "artistId"}, {ArtistNameRole, "artistName"}};
    }

private:
    SessionManager* m_session;
};
