#pragma once

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QPair>

namespace flowdriver::ui {

class QueryModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        ValueRole
    };

    explicit QueryModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void addQuery();
    Q_INVOKABLE void removeQuery(int index);
    Q_INVOKABLE void setKey(int index, const QString& key);
    Q_INVOKABLE void setValue(int index, const QString& value);
    Q_INVOKABLE QString buildQueryString() const;

    Q_INVOKABLE void clear();

private:
    QList<QPair<QString, QString>> m_queries;
};

} // namespace flowdriver::ui 