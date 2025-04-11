#include "models/auth_model.hpp"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace flowdriver::ui {

AuthModel::AuthModel(QObject* parent)
    : QObject(parent)
    , m_authType(0)
    , m_username()
    , m_password()
    , m_token()
    , m_apiKeyName()
    , m_apiKeyValue()
    , m_apiKeyLocation(0)
{
}

void AuthModel::setAuthType(int type) {
    if (m_authType != type) {
        m_authType = type;
        emit authTypeChanged();
    }
}

void AuthModel::setUsername(const QString& username) {
    if (m_username != username) {
        m_username = username;
        emit usernameChanged();
    }
}

void AuthModel::setPassword(const QString& password) {
    if (m_password != password) {
        m_password = password;
        emit passwordChanged();
    }
}

void AuthModel::setToken(const QString& token) {
    if (m_token != token) {
        m_token = token;
        emit tokenChanged();
    }
}

void AuthModel::setApiKeyName(const QString& name) {
    if (m_apiKeyName != name) {
        m_apiKeyName = name;
        emit apiKeyNameChanged();
    }
}

void AuthModel::setApiKeyValue(const QString& value) {
    if (m_apiKeyValue != value) {
        m_apiKeyValue = value;
        emit apiKeyValueChanged();
    }
}

void AuthModel::setApiKeyLocation(int location) {
    if (m_apiKeyLocation != location) {
        m_apiKeyLocation = location;
        emit apiKeyLocationChanged();
    }
}

QVariantList AuthModel::getAuthHeaders() const {
    QVariantList headers;
    QVariantMap header;

    switch (m_authType) {
        case 1: // Basic
            {
                QString auth = QString("%1:%2").arg(m_username, m_password);
                QByteArray base64Auth = auth.toUtf8().toBase64();
                header["name"] = "Authorization";
                header["value"] = "Basic " + QString::fromLatin1(base64Auth);
                headers.append(header);
            }
            break;

        case 2: // Bearer
            if (!m_token.isEmpty()) {
                header["name"] = "Authorization";
                header["value"] = "Bearer " + m_token;
                headers.append(header);
            }
            break;

        case 3: // API Key
            if (!m_apiKeyValue.isEmpty() && !m_apiKeyName.isEmpty() && m_apiKeyLocation == 0) {
                header["name"] = m_apiKeyName;
                header["value"] = m_apiKeyValue;
                headers.append(header);
            }
            break;

        default: // None or invalid
            break;
    }

    return headers;
}

} // namespace flowdriver::ui
