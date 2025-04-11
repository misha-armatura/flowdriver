#pragma once

#include <QObject>
#include <QString>
#include <QDebug>

namespace flowdriver::ui {

class BodyModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString content READ getContent WRITE setContent NOTIFY contentChanged)
    Q_PROPERTY(QString contentType READ getContentType WRITE setContentType NOTIFY contentTypeChanged)

public:
    explicit BodyModel(QObject* parent = nullptr);

    Q_INVOKABLE QString getContent() const { return m_content; }
    Q_INVOKABLE void setContent(const QString& content);

    Q_INVOKABLE QString getContentType() const { return m_contentType; }
    Q_INVOKABLE void setContentType(const QString& type);

    Q_INVOKABLE void formatJson();

    Q_INVOKABLE void clear();

signals:
    void contentChanged();
    void contentTypeChanged();
    void errorOccurred(const QString& error);

private:
    QString m_content;
    QString m_contentType{"application/json"};
};

} // namespace flowdriver::ui 
