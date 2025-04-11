#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

namespace flowdriver::ui {

class AuthModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int authType READ authType WRITE setAuthType NOTIFY authTypeChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
    Q_PROPERTY(QString apiKeyName READ apiKeyName WRITE setApiKeyName NOTIFY apiKeyNameChanged)
    Q_PROPERTY(QString apiKeyValue READ apiKeyValue WRITE setApiKeyValue NOTIFY apiKeyValueChanged)
    Q_PROPERTY(int apiKeyLocation READ apiKeyLocation WRITE setApiKeyLocation NOTIFY apiKeyLocationChanged)

public:
    explicit AuthModel(QObject* parent = nullptr);

    int authType() const { return m_authType; }
    QString username() const { return m_username; }
    QString password() const { return m_password; }
    QString token() const { return m_token; }
    QString apiKeyName() const { return m_apiKeyName; }
    QString apiKeyValue() const { return m_apiKeyValue; }
    int apiKeyLocation() const { return m_apiKeyLocation; }

    void setAuthType(int type);
    void setUsername(const QString& username);
    void setPassword(const QString& password);
    void setToken(const QString& token);
    void setApiKeyName(const QString& name);
    void setApiKeyValue(const QString& value);
    void setApiKeyLocation(int location);

    QVariantList getAuthHeaders() const;

    Q_INVOKABLE void clear() {
        setAuthType(0);
        setUsername("");
        setPassword("");
        setToken("");
        setApiKeyName("");
        setApiKeyValue("");
        setApiKeyLocation(0);
    }

signals:
    void authTypeChanged();
    void usernameChanged();
    void passwordChanged();
    void tokenChanged();
    void apiKeyNameChanged();
    void apiKeyValueChanged();
    void apiKeyLocationChanged();

private:
    int m_authType{0};  // 0: None, 1: Basic, 2: Bearer, 3: API Key
    QString m_username;
    QString m_password;
    QString m_token;
    QString m_apiKeyName;
    QString m_apiKeyValue;
    int m_apiKeyLocation{0};  // 0: Header, 1: Query Parameter
};

} // namespace flowdriver::ui
