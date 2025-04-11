#include "models/request_manager.hpp"
#include "core/rest_handler.hpp"
#include "core/websocket_handler.hpp"
#include "core/zeromq_handler.hpp"
#include <QVariantMap>
#include <QTimer>
#include <iostream>
#include <QDebug>
#include <future>
#include <chrono>
#include <QFile>
#include <QTextStream>
#include <nlohmann/json.hpp>

namespace flowdriver::ui {

RequestManager::RequestManager(QObject* parent)
    : QObject(parent)
    , m_handler(nullptr)
    , m_isLoading(false)
    , m_currentProtocol("REST")  // We start with REST by default
    , m_isConnected(false)
    , m_protoFilePath("")
{
    qDebug() << "RequestManager initializing...";
    initializeProtocolHandler(); 
    qDebug() << "RequestManager initialized with" << m_currentProtocol << "protocol";
}

void RequestManager::setCurrentProtocol(const QString& protocol) {
    if (m_currentProtocol != protocol) {
        qDebug() << "Switching protocol from" << m_currentProtocol << "to" << protocol;
        
        if (m_zmqHandler) {
            m_zmqHandler->cancel();
            m_zmqHandler.reset();
        }
        if (m_wsHandler) {
            m_wsHandler->cancel();
            m_wsHandler.reset();
        }
        if (m_restHandler) {
            m_restHandler.reset();
        }
        if (m_grpcHandler) {
            m_grpcHandler.reset();
        }
        
        m_currentProtocol = protocol;
        m_isConnected = false;
        
        // FIXME need to implement this more correctly
        if (protocol == "gRPC") {
            m_grpcHandler = std::make_unique<GrpcHandler>();
            m_handler = m_grpcHandler.get();
            // If we already have a proto file path, load it
            if (!m_protoFilePath.isEmpty()) {
                try {
                    loadGrpcProtoFile(m_protoFilePath);
                } catch (const std::exception& e) {
                    emit errorOccurred(QString("Failed to load proto file: %1").arg(e.what()));
                }
            }
        } else {
            initializeProtocolHandler();  // Handle other protocols
        }
        
        emit protocolChanged();
        emit connectionStatusChanged();
    }
}

void RequestManager::setCurrentRole(const QString &role)
{
    if (m_currentRole != role) {
        m_currentRole = role;
        emit currentRoleChanged();
    }
}

void RequestManager::setAuthModel(AuthModel* model) {
    if (m_authModel != model) {
        m_authModel = model;
        emit authModelChanged();
    }
}

bool RequestManager::validateRestRequest(const QString& method, const QString& url) {
    // Validate URL
    if (url.isEmpty()) {
        emit errorOccurred("URL cannot be empty");
        return false;
    }
    
    // Basic URL validation
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        emit errorOccurred("URL must start with http:// or https://");
        return false;
    }
    
    // Validate method
    const QStringList validMethods = {"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"};
    if (!validMethods.contains(method)) {
        emit errorOccurred("Invalid HTTP method: " + method);
        return false;
    }
    
    return true;
}

void RequestManager::executeRequest(const QString& method, const QString& url, const QVariantList& headers, const QString& body) {
    if (m_isLoading) {
        emit errorOccurred("A request is already in progress");
        return;
    }
    
    if (!validateRestRequest(method, url)) {
        return;
    }
    
    try {
        m_isLoading = true;
        emit loadingChanged();
        
        if (m_currentProtocol != "REST") {
            setCurrentProtocol("REST");
        }
        
        RequestConfig config = prepareConfig(method, url, headers, body);
        
        if (m_authModel) {
            auto authHeaders = m_authModel->getAuthHeaders();
            for (const auto& header : authHeaders) {
                config.headers.push_back({
                    header.toMap()["name"].toString().toStdString(),
                    header.toMap()["value"].toString().toStdString()
                });
            }
        }
        
        m_currentRequest = m_restHandler->executeAsync(config);
        
        // Timer to check for completion of request
        QTimer::singleShot(COMPLETION_TIMEOUT, this, &RequestManager::checkRequest);
        
        qDebug() << "Executing REST request:" << method << url;
        
    } catch (const Error& e) {
        m_isLoading = false;
        emit loadingChanged();
        emit errorOccurred(QString::fromStdString(e.what()));
    } catch (const std::exception& e) {
        m_isLoading = false;
        emit loadingChanged();
        emit errorOccurred(QString("Error: %1").arg(e.what()));
    }
}

void RequestManager::cancelRequest() {
    if (m_handler && m_isLoading) {
        m_handler->cancel();
        m_isLoading = false;
        emit loadingChanged();
    }
}

void RequestManager::initializeProtocolHandler() {
    qDebug() << "Initializing protocol handler for:" << m_currentProtocol;
    
    if (m_zmqHandler) {
        m_zmqHandler->cancel();
        m_zmqHandler.reset();
    }
    if (m_wsHandler) {
        m_wsHandler->cancel();
        m_wsHandler.reset();
    }
    if (m_restHandler) {
        m_restHandler.reset();
    }
    
    m_handler = nullptr;
    m_isConnected = false;

    try {
        if (m_currentProtocol == "REST") {
            m_restHandler = std::make_unique<RestHandler>();
            m_handler = m_restHandler.get();
            qDebug() << "REST handler initialized";
        } else if (m_currentProtocol == "WebSocket") {
            m_wsHandler = std::make_unique<WebSocketHandler>(this);
            QObject::connect(m_wsHandler.get(), &WebSocketHandler::connected,
                    this, &RequestManager::onWebSocketConnected);
            QObject::connect(m_wsHandler.get(), &WebSocketHandler::disconnected,
                    this, &RequestManager::onWebSocketDisconnected);
            QObject::connect(m_wsHandler.get(), &WebSocketHandler::messageReceived,
                    [this](const std::string& msg) {
                        QVariantMap response;
                        response["body"] = QString::fromStdString(msg);
                        response["status_code"] = 200;
                        emit responseReceived(response);
                    });
            m_handler = m_wsHandler.get();
            qDebug() << "WebSocket handler initialized";
        } else if (m_currentProtocol == "ZeroMQ") {
            if (!m_zmqHandler) {
                m_zmqHandler = std::make_unique<ZeroMQHandler>(this);
                
                connect(m_zmqHandler.get(), &ZeroMQHandler::messageReceived,
                        this, [this](const QString& message) {
                            qDebug() << "ZMQ message received:" << message;
                           
                            if (m_currentRole == "REPLIER" ||
                                m_currentRole == "SUBSCRIBER" ||
                                m_currentRole == "PULLER" ||
                                m_currentRole == "ROUTER") {
                                QVariantMap response;
                                response["body"] = message;
                                response["status_code"] = 200;
                                emit responseReceived(response);
                            }
                        });
                
                connect(m_zmqHandler.get(), &ZeroMQHandler::errorOccurred,
                        this, &RequestManager::errorOccurred);
            }
            m_handler = m_zmqHandler.get();
            qDebug() << "ZeroMQ handler initialized";
        }
        emit protocolChanged();
    } catch (const std::exception& e) {
        qDebug() << "Error initializing protocol handler:" << e.what();
        emit errorOccurred(QString("Failed to initialize protocol: %1").arg(e.what()));
    }
}

RequestConfig RequestManager::prepareConfig(const QString& method, const QString& url,
                                          const QVariantList& headers, const QString& body) {
    RequestConfig config;
    config.method = method.toStdString();
    config.url = url.toStdString();

    for (const auto& header : headers) {
        auto map = header.toMap();
        config.headers.push_back({
            map["name"].toString().toStdString(),
            map["value"].toString().toStdString()
        });
    }

    if (!body.isEmpty()) {
        config.body = body.toStdString();
    }

    return config;
}

void RequestManager::handleZMQMessage(const std::string& message) {
    QVariantMap response;
    response["body"] = QString::fromStdString(message);
    response["status_code"] = 200;
    response["content_type"] = "application/json";

    emit responseReceived(response);
}

void RequestManager::executeZMQRequest(const QString& method, const QString& endpoint,
                                     const QString& role, const QString& body) {
    qDebug() << "Executing ZMQ request:" << method << endpoint << role << body;
    
    if (!m_zmqHandler) {
        qDebug() << "ZMQ handler not initialized!";
        emit errorOccurred("ZMQ handler not initialized");
        return;
    }

    try {
        RequestConfig config;
        config.url = endpoint.toStdString();
        config.body = body.toStdString();
        config.method = method.toStdString();
        
        qDebug() << "Executing ZMQ request with config";
        
        emit messageSent(body);
        
        auto result = m_zmqHandler->execute(config);
        
        ///TODO fixit
        if (!result.body.empty() && role == "REQUESTER") {
            QVariantMap response;
            response["body"] = QString::fromStdString(result.body);
            response["status_code"] = 200;
            emit responseReceived(response);
        }
        
    } catch (const Error& e) {
        qDebug() << "Error executing ZMQ request:" << e.what();
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void RequestManager::handleZMQError(const QString& error) {
    emit errorOccurred(QString("ZMQ Error: %1").arg(error));
}

void RequestManager::connectWebSocket(const QString& url) {
    if (!m_wsHandler) {
        emit errorOccurred("WebSocket handler not initialized");
        return;
    }
    
    if (!url.startsWith("ws://") && !url.startsWith("wss://")) {
        emit errorOccurred("WebSocket URL must start with ws:// or wss://");
        return;
    }
    
    try {
        RequestConfig config;
        config.url = url.toStdString();
        
        if (m_authModel) {
            auto authHeaders = m_authModel->getAuthHeaders();
            for (const auto& header : authHeaders) {
                auto map = header.toMap();
                config.headers.push_back({
                    map["name"].toString().toStdString(),
                    map["value"].toString().toStdString()
                });
            }
        }
        
        m_wsHandler->connect(config);
        
    } catch (const std::exception& e) {
        qDebug() << "Failed to connect WebSocket:" << e.what();
        emit errorOccurred("Failed to establish WebSocket connection");
    }
}

void RequestManager::disconnect() {
    if (m_wsHandler) {
        m_wsHandler->cancel();
    }
    if (m_zmqHandler) {
        m_zmqHandler->cancel();
    }
    
    m_isConnected = false;
    emit connectionStatusChanged();
    emit disconnected();
}

void RequestManager::sendWebSocketMessage(const QString& message) {
    if (!m_isConnected) {
        emit errorOccurred("Not connected");
        return;
    }

    try {
        if (auto ws = dynamic_cast<WebSocketHandler*>(m_handler)) {
            RequestConfig config;
            config.body = message.toStdString();
            ws->executeAsync(config);
            emit messageSent(message);
        }
    } catch (const Error& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void RequestManager::executeGrpcRequest(const QString& requestBody) {
    qDebug() << "Starting gRPC request execution";
    qDebug() << "Request body:" << requestBody;
    qDebug() << "Current service:" << m_currentGrpcService;
    qDebug() << "Current method:" << m_currentGrpcMethod;
    qDebug() << "Endpoint:" << m_grpcEndpoint;

    if (!m_grpcHandler) {
        QString error = "gRPC handler not initialized";
        qDebug() << "Error:" << error;
        emit errorOccurred(error);
        return;
    }
    
    if (m_grpcEndpoint.isEmpty()) {
        QString error = "gRPC endpoint not set";
        qDebug() << "Error:" << error;
        emit errorOccurred(error);
        return;
    }

    if (m_isLoading) {
        QString error = "Request already in progress";
        qDebug() << "Error:" << error;
        emit errorOccurred(error);
        return;
    }
    
    try {
        m_isLoading = true;
        emit loadingChanged();
        
        RequestConfig config;
        config.body = requestBody.isEmpty() ? "{}" : requestBody.toStdString();
        config.url = m_grpcEndpoint.toStdString();
        
        qDebug() << "Executing gRPC request with config:";
        qDebug() << "- URL:" << QString::fromStdString(config.url);
        qDebug() << "- Body:" << QString::fromStdString(config.body);
        
        RequestResult result = m_grpcHandler->execute(config);
        
        if (result.status_code != 200) {
            QString errorMsg = QString::fromStdString(result.error);
            if (errorMsg.isEmpty()) {
                errorMsg = QString("gRPC request failed with status code: %1").arg(result.status_code);
            }
            emit errorOccurred(errorMsg);
        }
        
        QVariantMap response = convertResultToVariantMap(result);
        m_lastResponse = response;
        emit responseReceived(response);
        
    } catch (const Error& e) {
        QString error = QString::fromStdString(e.what());
        qDebug() << "Error executing gRPC request:" << error;
        emit errorOccurred(error);
    } catch (const std::exception& e) {
        QString error = QString("Unexpected error: %1").arg(e.what());
        qDebug() << error;
        emit errorOccurred(error);
    }
    
    m_isLoading = false;
    emit loadingChanged();
}

void RequestManager::checkRequest() {
    if (!m_currentRequest.valid()) {
        m_isLoading = false;
        emit loadingChanged();
        return;
    }

    auto status = m_currentRequest.wait_for(std::chrono::milliseconds(0));
    if (status == std::future_status::ready) {
        try {
            RequestResult result = m_currentRequest.get();

            qDebug() << "Request result:";
            qDebug() << "- Status code:" << result.status_code;
            qDebug() << "- Body:" << QString::fromStdString(result.body);
            qDebug() << "- Error:" << QString::fromStdString(result.error);


            // Convert to QVariantMap and emit signal
            QVariantMap response = convertResultToVariantMap(result);
            m_lastResponse = response;  // Store the last response
            emit responseReceived(response);

            qDebug() << "Request completed with status code:" << result.status_code;
            qDebug() << "Response body:" << QString::fromStdString(result.body);

        } catch (const std::exception& e) {
            qDebug() << "Error getting request result:" << e.what();
            emit errorOccurred(QString("Error: %1").arg(e.what()));
        }

        m_isLoading = false;
        emit loadingChanged();
    } else {
        qDebug() << "Request not ready yet, checking again in 100ms";

        QTimer::singleShot(CHECK_REQUEST_TIMEOUT, this, &RequestManager::checkRequest);
    }
}

void RequestManager::connectZMQ(const QString& endpoint, const QString& pattern, const QString& role) {
    try {
        auto zmqPattern = convertPattern(pattern);
        auto zmqRole = convertZMQRole(role);
        
        if (!m_zmqHandler) {
            throw Error(ErrorCode::INVALID_STATE, "ZMQ handler not initialized");
        }

        m_zmqHandler->configure(zmqPattern, zmqRole, endpoint.toStdString());
        m_currentRole = role;
        
        m_isConnected = true;
        emit connectionStatusChanged();
        emit connected();
        
    } catch (const Error& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

void RequestManager::loadGrpcProtoFile(const QString& path) {
    try {
        if (!m_grpcHandler) {
            m_grpcHandler = std::make_unique<GrpcHandler>();
            m_handler = m_grpcHandler.get();
        }
        
        m_grpcHandler->loadProtoFile(path.toStdString());
        
        // Update available services
        m_grpcServices = m_grpcHandler->getAvailableServices();
        
        // Clear existing service methods
        m_grpcServiceMethods.clear();
        
        // Cache methods for each service
        for (const auto& service : m_grpcServices) {
            QStringList methods = m_grpcHandler->getServiceMethods(service.toStdString());
            m_grpcServiceMethods[service] = methods;
            qDebug() << "Loaded service:" << service << "with methods:" << methods;
        }
        
        // Set initial service if available
        if (!m_grpcServices.isEmpty()) {
            setGrpcService(m_grpcServices.first());
        }

        emit grpcServicesChanged();
    } catch (const Error& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

QStringList RequestManager::getGrpcMethods(const QString& service) {
    QStringList methods = m_grpcServiceMethods.value(service, QStringList());
    qDebug() << "Getting methods for service:" << service << "found:" << methods;
    return methods;
}

void RequestManager::setGrpcService(const QString& service) {
    qDebug() << "Setting gRPC service to:" << service;
    if (m_currentGrpcService != service) {
        m_currentGrpcService = service;
        if (m_grpcHandler) {
            try {
                m_grpcHandler->setService(service.toStdString());
                QStringList methods = m_grpcHandler->getServiceMethods(service.toStdString());
                m_grpcServiceMethods[service] = methods;
                qDebug() << "Updated methods for service" << service << ":" << methods;
            } catch (const Error& e) {
                emit errorOccurred(QString::fromStdString(e.what()));
            }
        }
    }
}

void RequestManager::setGrpcMethod(const QString& method) {
    if (m_currentGrpcMethod != method) {
        m_currentGrpcMethod = method;
        if (m_grpcHandler) {
            m_grpcHandler->setMethod(method.toStdString());
        }
    }
}

void RequestManager::setGrpcEndpoint(const QString& endpoint) {
    if (m_grpcEndpoint != endpoint) {
        m_grpcEndpoint = endpoint;
        if (m_grpcHandler) {
            try {
                m_grpcHandler->setEndpoint(endpoint.toStdString());
                emit grpcEndpointChanged();
            } catch (const Error& e) {
                emit errorOccurred(QString::fromStdString(e.what()));
            }
        }
    }
}

void RequestManager::setGrpcUseSSL(bool use) {
    if (m_grpcUseSSL != use) {
        m_grpcUseSSL = use;
        if (m_grpcHandler) {
            m_grpcHandler->setUseSSL(use);
        }
        emit grpcUseSSLChanged();
    }
}

void RequestManager::setProtoFilePath(const QString& path) {
    if (m_protoFilePath != path) {
        m_protoFilePath = path;
        if (m_grpcHandler) {
            try {
                loadGrpcProtoFile(path);
            } catch (const std::exception& e) {
                emit errorOccurred(QString("Failed to load proto file: %1").arg(e.what()));
            }
        }
        emit protoFilePathChanged();
    }
}

void RequestManager::exportResponse(const QString& format) {
    if (!m_lastResponse.contains("body") || m_lastResponse["body"].toString().isEmpty()) {
        emit errorOccurred("No response to export");
        return;
    }
    
    emit exportRequested(format, m_lastResponse);
}

Q_INVOKABLE void RequestManager::saveResponseToFile(const QString& filePath, const QString& format, const QVariantMap& response) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred("Failed to open file for writing");
        return;
    }
    
    QTextStream out(&file);
    
    if (format == "JSON") {
        out << response["body"].toString();
    } else if (format == "CSV") {
        QString body = response["body"].toString();
        try {
            auto json = nlohmann::json::parse(body.toStdString());
            if (json.is_array()) {
                if (!json.empty() && json[0].is_object()) {
                    // Write header
                    bool first = true;
                    for (auto& [key, value] : json[0].items()) {
                        if (!first) out << ",";
                        out << QString::fromStdString(key);
                        first = false;
                    }
                    out << "\n";
                    
                    // Write data
                    for (auto& item : json) {
                        first = true;
                        for (auto& [key, value] : item.items()) {
                            if (!first) out << ",";
                            out << QString::fromStdString(value.dump());
                            first = false;
                        }
                        out << "\n";
                    }
                }
            }
        } catch (const std::exception& e) {
            emit errorOccurred("Failed to parse JSON for CSV export");
        }
    } else if (format == "HTML") {
        out << "<!DOCTYPE html>\n<html>\n<head>\n";
        out << "<title>FlowDriver Response</title>\n";
        out << "<style>body{font-family:sans-serif;margin:20px}pre{background:#f5f5f5;padding:10px;border-radius:5px}</style>\n";
        out << "</head>\n<body>\n";
        out << "<h1>Response</h1>\n";
        out << "<h2>Status: " << response["status_code"].toInt() << "</h2>\n";
        out << "<h3>Headers:</h3>\n<ul>\n";
        
        for (const QVariant& header : response["headers"].toList()) {
            QVariantMap headerMap = header.toMap();
            out << "<li><strong>" << headerMap["name"].toString() << ":</strong> " 
                << headerMap["value"].toString() << "</li>\n";
        }
        
        out << "</ul>\n<h3>Body:</h3>\n";
        out << "<pre>" << response["body"].toString() << "</pre>\n";
        out << "</body>\n</html>";
    } else if (format == "PDF") {
        emit errorOccurred("PDF export not implemented yet");
    }
    
    file.close();
    emit exportCompleted();
}

void RequestManager::clearMessages() {
    emit messagesCleared();
}

} // namespace flowdriver::ui 
