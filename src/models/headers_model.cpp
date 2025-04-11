#include "models/headers_model.hpp"

namespace flowdriver::ui {

HeadersModel::HeadersModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int HeadersModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    
    return m_headers.size();
}

QVariant HeadersModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_headers.size())  return QVariant();

    const Header& header = m_headers[index.row()];

    switch (role) {
        case NameRole:
            return header.name;
        case ValueRole:
            return header.value;
        default:
            return QVariant();
    }
}

bool HeadersModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.row() >= m_headers.size())
        return false;

    Header& header = m_headers[index.row()];

    switch (role) {
        case NameRole:
            header.name = value.toString();
            break;
        case ValueRole:
            header.value = value.toString();
            break;
        default:
            return false;
    }

    emit dataChanged(index, index, {role});
    emit headersChanged();
    return true;
}

QHash<int, QByteArray> HeadersModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[ValueRole] = "value";
    return roles;
}

QVariantList HeadersModel::getHeaders() const {
    QVariantList result;
    for (const auto& header : m_headers) {
        QVariantMap map;
        map["name"] = header.name;
        map["value"] = header.value;
        result.append(map);
    }
    return result;
}

void HeadersModel::addHeader() {
    beginInsertRows(QModelIndex(), m_headers.size(), m_headers.size());
    m_headers.push_back({"", ""});
    endInsertRows();
    emit headersChanged();
}

void HeadersModel::removeHeader(int index) {
    if (index < 0 || index >= m_headers.size())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_headers.erase(m_headers.begin() + index);
    endRemoveRows();
    emit headersChanged();
}

void HeadersModel::clear() {
    beginResetModel();
    m_headers.clear();
    endResetModel();
    emit headersChanged();
}

void HeadersModel::updateValue(int index, const QString& name, const QString& value) {
    if (index < 0 || index >= m_headers.size())
        return;

    m_headers[index].name = name;
    m_headers[index].value = value;
    
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {NameRole, ValueRole});
    emit headersChanged();
}

void HeadersModel::append(const QString& name, const QString& value) {
    beginInsertRows(QModelIndex(), m_headers.size(), m_headers.size());
    m_headers.push_back({name, value});
    endInsertRows();
    emit headersChanged();
}

void HeadersModel::remove(int index) {
    if (index < 0 || index >= m_headers.size())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_headers.erase(m_headers.begin() + index);
    endRemoveRows();
    emit headersChanged();
}

} // namespace flowdriver::ui 
