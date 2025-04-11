#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QObject>
#include <vector>

namespace flowdriver::ui {

class HeadersModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QVariantList headers READ getHeaders NOTIFY headersChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ValueRole
    };

    explicit HeadersModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void append(const QString& name = "", const QString& value = "");
    Q_INVOKABLE void remove(int index);
    Q_INVOKABLE void updateValue(int index, const QString& name, const QString& value);

    QVariantList getHeaders() const;

public slots:
    void addHeader();
    void removeHeader(int index);
    void clear();

signals:
    void headersChanged();

private:
    struct Header {
        QString name;
        QString value;
    };
    std::vector<Header> m_headers;
};

} // namespace flowdriver::ui 