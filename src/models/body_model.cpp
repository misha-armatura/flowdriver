#pragma once

#include "models/body_model.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>


namespace flowdriver::ui {

BodyModel::BodyModel(QObject* parent) 
    : QObject(parent)
    , m_content()
    , m_contentType()
{
    qDebug() << "BodyModel constructed, this:" << this;
}

void BodyModel::setContent(const QString& content) {
    if (m_content != content) {
        qDebug() << "BodyModel::setContent called with:" << content << "this:" << this;
        m_content = content;
        emit contentChanged();
    }
}

void BodyModel::setContentType(const QString& type) {
    if (m_contentType != type) {
        m_contentType = type;
        emit contentTypeChanged();
    }
}

void BodyModel::clear() {
    setContent("");
    setContentType("");
}

void BodyModel::formatJson() {
    if (m_content.isEmpty()) {
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(m_content.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        emit errorOccurred(tr("Invalid JSON: %1").arg(error.errorString()));
        return;
    }

    setContent(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
}

} // namespace flowdriver::ui 
