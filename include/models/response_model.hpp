#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <chrono>
#include "core/types.hpp"

namespace flowdriver::ui {

#define RESPONSE_TIMEOUT 1000

class ResponseModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int statusCode READ getStatusCode NOTIFY responseChanged)
    Q_PROPERTY(QString body READ getBody NOTIFY responseChanged)
    Q_PROPERTY(QString formattedBody READ getFormattedBody NOTIFY responseChanged)
    Q_PROPERTY(QVariantList headers READ getHeaders NOTIFY responseChanged)
    Q_PROPERTY(QString error READ getError NOTIFY responseChanged)
    Q_PROPERTY(QString time READ getTime NOTIFY responseChanged)

public:
    explicit ResponseModel(QObject* parent = nullptr);

    int getStatusCode() const { return m_statusCode; }
    QString getBody() const { return m_body; }
    QString getFormattedBody() const;
    QVariantList getHeaders() const { return m_headers; }
    QString getError() const { return m_error; }
    QString getTime() const;

    Q_INVOKABLE void updateResponse(const QVariantMap& response);
    Q_INVOKABLE void clear();

signals:
    void responseChanged();

private:
    void parseHeaders(const std::vector<Header>& headers);
    void parseCookies(const std::vector<Header>& headers);
    QString formatJson(const QString& json) const;

private:
    int m_statusCode{0};
    QString m_body;
    QVariantList m_headers;
    QVariantList m_cookies;
    QString m_error;
    std::chrono::microseconds m_responseTime{0};
};

} // namespace flowdriver::ui 