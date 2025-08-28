#include "sessionartistmodel.h"

SessionArtistModel::SessionArtistModel(ArtistService* artistService, QObject* parent)
    : QAbstractListModel(parent) {

    m_session = (artistService->sessionManager());

    connect(m_session, &SessionManager::artistAdded, this, [=](const Artist&){
        int newRow = m_session->artists().size() - 1;
        beginInsertRows(QModelIndex(), newRow, newRow);
        endInsertRows();
    });

    connect(m_session, &SessionManager::artistRemoved, this, [=](const Artist&){
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
    if (role == ArtistIdRole) return sessionArtist.id;
    return QVariant();
}

void SessionArtistModel::removeSessionArtistByListIndex(const int listIndex) {
    m_session->removeArtistByListIndex(listIndex);
}

/*QHash<int, QByteArray> SessionArtistModel::roleNames() const {
    return {
        {IdRole, "id"},
        {NameRole, "name"}
    };
}
*/
