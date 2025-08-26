// SessionArtistModel.h
#pragma once
#include <QAbstractListModel>

#include <QMetaEnum>
#include "sessionmanager.h"

class SessionArtistModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit SessionArtistModel(SessionManager* session, QObject* parent = nullptr);

    //enum Roles { IdRole = Qt::UserRole + 1, NameRole };
    enum Roles { ArtistNameRole = Qt::UserRole + 1 };
    Q_ENUM(Roles);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    //QHash<int, QByteArray> roleNames() const override;
    QHash<int, QByteArray> roleNames() const override {
        return { {ArtistNameRole, "artistName"} };
    }
private:
    SessionManager* m_session;
};
