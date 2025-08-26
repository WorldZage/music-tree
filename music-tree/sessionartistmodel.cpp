#include "sessionartistmodel.h"

SessionArtistModel::SessionArtistModel(SessionManager* session, QObject* parent)
    : QAbstractListModel(parent), m_session(session) {

    connect(m_session, &SessionManager::artistAdded, this, [=](const SessionArtist&){
        int newRow = m_session->artists().size() - 1;
        beginInsertRows(QModelIndex(), newRow, newRow);
        endInsertRows();
    });

    connect(m_session, &SessionManager::artistRemoved, this, [=](const QString&){
        beginResetModel(); endResetModel(); // simple but works
    });
    connect(m_session, &SessionManager::sessionCleared, this, [=](){
        beginResetModel(); endResetModel();
    });
}

int SessionArtistModel::rowCount(const QModelIndex&) const {
    return m_session->artists().size();
}

QVariant SessionArtistModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_session->artists().size())
        return QVariant();

    const auto& sessionArtist = m_session->artists().at(index.row());
    if (role == ArtistNameRole) return sessionArtist.name;
    return QVariant();
}

/*QHash<int, QByteArray> SessionArtistModel::roleNames() const {
    return {
        {IdRole, "id"},
        {NameRole, "name"}
    };
}
*/
