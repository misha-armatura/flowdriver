#include "models/response_model.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <nlohmann/json.hpp>

namespace flowdriver::ui {

using json = nlohmann::json;

ResponseModel::ResponseModel(QObject* parent)
    : QObject(parent)
{
}

QString ResponseModel::getFormattedBody() const {
    // Try to format as JSON if possible
    if (m_body.startsWith('{') || m_body.startsWith('[')) {
        return formatJson(m_body);
    }
    return m_body;
}

QString ResponseModel::getTime() const {
    if (m_responseTime.count() > RESPONSE_TIMEOUT) {
        return QString::number(static_cast<float>(m_responseTime.count() / RESPONSE_TIMEOUT), 'f', 2) + " ms";
    }
    return QString::number(m_responseTime.count()) + " μs";
}

void ResponseModel::updateResponse(const QVariantMap& response) {
    m_statusCode = response["status_code"].toInt();
    m_body = response["body"].toString();
    m_headers = response["headers"].toList();
    m_error = response["error"].toString();
    
    if (response.contains("time")) {
        m_responseTime = std::chrono::microseconds(response["time"].toLongLong());
    }
    
    emit responseChanged();
}

void ResponseModel::clear() {
    m_body.clear();
    m_statusCode = 0;
    m_headers.clear();
    m_cookies.clear();
    m_responseTime = std::chrono::microseconds(0);
    m_error.clear();
    
    emit responseChanged();
}

void ResponseModel::parseHeaders(const std::vector<Header>& headers) {
    m_headers.clear();
    for (const auto& header : headers) {
        QVariantMap headerMap;
        headerMap["name"] = QString::fromStdString(header.name);
        headerMap["value"] = QString::fromStdString(header.value);
        m_headers.append(headerMap);
    }
}

void ResponseModel::parseCookies(const std::vector<Header>& headers) {
    m_cookies.clear();
    for (const auto& header : headers) {
        if (QString::fromStdString(header.name).toLower() == "set-cookie") {
            QVariantMap cookieMap;
            QString value = QString::fromStdString(header.value);
            
            auto parts = value.split(';');
            if (!parts.isEmpty()) {
                auto nameValue = parts[0].split('=');
                if (nameValue.size() >= 2) {
                    cookieMap["name"] = nameValue[0].trimmed();
                    cookieMap["value"] = nameValue[1].trimmed();
                    m_cookies.append(cookieMap);
                }
            }
        }
    }
}

QString ResponseModel::formatJson(const QString& json) const {
    try {
        return QString::fromStdString(json::parse(json.toStdString()).dump(2));
    } catch ([[__maybe_unused__]]const json::parse_error& jsn) {
        return json; // Return original if not valid JSON
    }
}

} // namespace flowdriver::ui 
