#pragma once

#include <QObject>
#include <QVariantMap>
#include <memory>
#include "core/protocol_handler.hpp"
#include "core/types.hpp"
#include "models/auth_model.hpp"
#include <core/zeromq_handler.hpp>
#include <core/rest_handler.hpp>
#include <core/websocket_handler.hpp>
#include <core/grpc_handler.hpp>

namespace flowdriver::ui {

#define COMPLETION_TIMEOUT 2000
#define CHECK_REQUEST_TIMEOUT 500

class RequestManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(QString currentProtocol READ getCurrentProtocol WRITE setCurrentProtocol NOTIFY protocolChanged)
    Q_PROPERTY(QString currentRole READ getCurrentRole WRITE setCurrentRole NOTIFY currentRoleChanged)
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(AuthModel* authModel READ getAuthModel WRITE setAuthModel NOTIFY authModelChanged)
    Q_PROPERTY(QStringList availableGrpcServices READ getAvailableGrpcServices NOTIFY grpcServicesChanged)
    Q_PROPERTY(QString grpcEndpoint READ getGrpcEndpoint WRITE setGrpcEndpoint NOTIFY grpcEndpointChanged)
    Q_PROPERTY(bool grpcUseSSL READ getGrpcUseSSL WRITE setGrpcUseSSL NOTIFY grpcUseSSLChanged)
    Q_PROPERTY(QString protoFilePath READ getProtoFilePath WRITE setProtoFilePath NOTIFY protoFilePathChanged)

public:
    explicit RequestManager(QObject* parent = nullptr);
    ~RequestManager() = default;

    RequestManager(const RequestManager&) = delete;
    RequestManager& operator=(const RequestManager&) = delete;

    bool isLoading() const { return m_isLoading; }
    QString getCurrentProtocol() const { return m_currentProtocol; }
    QString getCurrentRole() const { return m_currentRole; }
    ProtocolHandler* getHandler() { return m_handler; }
    bool isConnected() const { return m_isConnected; }
    AuthModel* getAuthModel() const { return m_authModel; }
    void setAuthModel(AuthModel* model);

    bool validateRestRequest(const QString& method, const QString& url);

    Q_INVOKABLE void exportResponse(const QString& format);

    /**
     * @brief Connect to WebSocket server
     * @param url WebSocket URL
     */
    Q_INVOKABLE void connectWebSocket(const QString& url);

    /**
     * @brief Execute gRPC request
     */
    Q_INVOKABLE void executeGrpcRequest(const QString& requestBody);

    /**
     * @brief Connect to ZeroMQ endpoint
     * @param endpoint ZeroMQ endpoint URL
     */
    Q_INVOKABLE void connectZMQ(const QString& endpoint, const QString& pattern, const QString& role);

    Q_INVOKABLE void loadGrpcProtoFile(const QString& path);
    Q_INVOKABLE QStringList getGrpcMethods(const QString& service);
    Q_INVOKABLE void setGrpcService(const QString& service);
    Q_INVOKABLE void setGrpcMethod(const QString& method);

    QStringList getAvailableGrpcServices() const { return m_grpcServices; }
    QString getGrpcEndpoint() const { return m_grpcEndpoint; }
    bool getGrpcUseSSL() const { return m_grpcUseSSL; }

    void setGrpcEndpoint(const QString& endpoint);
    void setGrpcUseSSL(bool use);

    QString getProtoFilePath() const { return m_protoFilePath; }
    void setProtoFilePath(const QString& path);

    Q_INVOKABLE void clearMessages();

public slots:
    void setCurrentProtocol(const QString& protocol);
    void setCurrentRole(const QString& role);
    void executeRequest(const QString& method, const QString& url, const QVariantList& headers, const QString& body);
    void executeZMQRequest(const QString& method, const QString& endpoint, const QString& role, const QString& body);
    void cancelRequest();
    void disconnect();
    void sendWebSocketMessage(const QString& message);
    Q_INVOKABLE void saveResponseToFile(const QString& filePath, const QString& format, const QVariantMap& response);

signals:
    void loadingChanged();
    void protocolChanged();
    void currentRoleChanged();
    void responseReceived(const QVariantMap& result);
    void errorOccurred(const QString& error);
    void messageSent(const QString& message);
    void messageReceived(const QString& message);
    void connected();
    void disconnected();
    void connectionStatusChanged();
    void authModelChanged();
    void grpcServicesChanged();
    void grpcEndpointChanged();
    void grpcUseSSLChanged();
    void protoFilePathChanged();
    void exportRequested(const QString& format, const QVariantMap& response);
    void exportCompleted();
    void messagesCleared();

private:
    void initializeProtocolHandler();
    RequestConfig prepareConfig(const QString& method, const QString& url, const QVariantList& headers, const QString& body);
    void checkRequest();

    ZeroMQHandler::Pattern convertPattern(const QString& pattern) {
        if (pattern == "REQ-REP") return ZeroMQHandler::Pattern::REQ_REP;
        if (pattern == "PUB-SUB") return ZeroMQHandler::Pattern::PUB_SUB;
        if (pattern == "PUSH-PULL") return ZeroMQHandler::Pattern::PUSH_PULL;
        if (pattern == "DEALER-ROUTER") return ZeroMQHandler::Pattern::DEALER_ROUTER;
        throw Error(ErrorCode::INVALID_CONFIG, "Invalid ZeroMQ pattern");
    }

    ZeroMQHandler::Role convertZMQRole(const QString& role) {
        if (role == "REQUESTER") return ZeroMQHandler::Role::REQUESTER;
        if (role == "REPLIER") return ZeroMQHandler::Role::REPLIER;
        if (role == "PUBLISHER") return ZeroMQHandler::Role::PUBLISHER;
        if (role == "SUBSCRIBER") return ZeroMQHandler::Role::SUBSCRIBER;
        if (role == "PUSHER") return ZeroMQHandler::Role::PUSHER;
        if (role == "PULLER") return ZeroMQHandler::Role::PULLER;
        if (role == "DEALER") return ZeroMQHandler::Role::DEALER;
        if (role == "ROUTER") return ZeroMQHandler::Role::ROUTER;
        throw Error(ErrorCode::INVALID_CONFIG, "Invalid ZeroMQ role");
    }

    // Helper to convert RequestResult to QVariantMap
    static QVariantMap convertResultToVariantMap(const RequestResult& result) {
        QVariantMap response;
        response["status_code"] = result.status_code;
        response["body"] = QString::fromStdString(result.body);
        response["error"] = QString::fromStdString(result.error);
        
        QVariantList headersList;
        for (const auto& header : result.headers) {
            QVariantMap headerMap;
            headerMap["name"] = QString::fromStdString(header.name);
            headerMap["value"] = QString::fromStdString(header.value);
            headersList.append(headerMap);
        }
        response["headers"] = headersList;
        
        return response;
    }

    bool m_isLoading{false};
    QString m_currentProtocol{"REST"};
    QString m_currentRole{"DEALER"};
    ProtocolHandler* m_handler{nullptr};
    std::future<RequestResult> m_currentRequest;
    std::unique_ptr<ZeroMQHandler> m_zmqHandler;
    std::unique_ptr<RestHandler> m_restHandler;
    std::unique_ptr<WebSocketHandler> m_wsHandler;
    bool m_isConnected{false};
    AuthModel* m_authModel{nullptr};
    std::unique_ptr<GrpcHandler> m_grpcHandler;
    QVariantMap m_lastResponse;

    void handleZMQMessage(const std::string& message);
    void handleZMQError(const QString& error);

    QStringList m_grpcServices;
    QString m_grpcEndpoint{"localhost:50051"};
    bool m_grpcUseSSL{false};
    QString m_currentGrpcService;
    QString m_currentGrpcMethod;
    QMap<QString, QStringList> m_grpcServiceMethods;

    QString m_protoFilePath;

private slots:
    void onWebSocketConnected() {
        m_isConnected = true;
        emit connectionStatusChanged();
        emit connected();
    }
    
    void onWebSocketDisconnected() {
        m_isConnected = false;
        emit connectionStatusChanged();
        emit disconnected();
    }
};

} // namespace flowdriver::ui 
