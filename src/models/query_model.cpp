#include "models/query_model.hpp"
#include <QUrl>
#include <QUrlQuery>

namespace flowdriver::ui {

QueryModel::QueryModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int QueryModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;

    return m_queries.size();
}

QVariant QueryModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_queries.size()) return QVariant();

    const auto& query = m_queries[index.row()];
    switch (role) {
        case KeyRole:
            return query.first;
        case ValueRole:
            return query.second;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> QueryModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[KeyRole] = "key";
    roles[ValueRole] = "value";
    return roles;
}

void QueryModel::addQuery() {
    beginInsertRows(QModelIndex(), m_queries.size(), m_queries.size());
    m_queries.append(QPair<QString, QString>("", ""));
    endInsertRows();
}

void QueryModel::removeQuery(int index) {
    if (index < 0 || index >= m_queries.size()) return;

    beginRemoveRows(QModelIndex(), index, index);
    m_queries.removeAt(index);
    endRemoveRows();
}

void QueryModel::setKey(int index, const QString& key) {
    if (index < 0 || index >= m_queries.size()) return;

    if (m_queries[index].first != key) {
        m_queries[index].first = key;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {KeyRole});
    }
}

void QueryModel::setValue(int index, const QString& value) {
    if (index < 0 || index >= m_queries.size()) return;

    if (m_queries[index].second != value) {
        m_queries[index].second = value;
        QModelIndex modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, {ValueRole});
    }
}

QString QueryModel::buildQueryString() const {
    QUrlQuery query;
    for (const auto& pair : m_queries) {
        if (!pair.first.isEmpty()) {
            query.addQueryItem(pair.first, pair.second);
        }
    }
    return query.toString();
}

void QueryModel::clear() {
    beginResetModel();
    m_queries.clear();
    endResetModel();
}

} // namespace flowdriver::ui 
