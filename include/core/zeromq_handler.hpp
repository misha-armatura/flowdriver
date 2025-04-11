#pragma once

#include "protocol_handler.hpp"
#include "core/error.hpp"
#include <zmq.hpp>
#include <string_view>
#include <memory>
#include <vector>
#include <string>
#include <QString>
#include <QObject>
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <map>

namespace flowdriver {

/**
 * @brief Handler for ZeroMQ protocol communications
 */
class ZeroMQHandler final : public ProtocolHandler {
    Q_OBJECT
public:
    /**
     * @brief Supported ZeroMQ patterns
     */
    enum class Pattern {
        REQ_REP,    // Request-Reply
        PUB_SUB,    // Publish-Subscribe
        PUSH_PULL,  // Pipeline
        DEALER_ROUTER // Advanced Request-Reply
    };
    Q_ENUM(Pattern)

    /**
     * @brief Supported ZeroMQ roles
     */
    enum class Role {
        // REQ-REP roles
        REQUESTER,
        REPLIER,
        // PUB-SUB roles
        PUBLISHER,
        SUBSCRIBER,
        // PUSH-PULL roles
        PUSHER,
        PULLER,
        // DEALER-ROUTER roles
        DEALER,
        ROUTER
    };
    Q_ENUM(Role)

    enum class ConnectionStatus {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        ERROR
    };

    static std::vector<Role> getAvailableRoles(const Pattern &pattern) {
        switch (pattern) {
            case Pattern::REQ_REP:
                return {Role::REQUESTER, Role::REPLIER};
            case Pattern::PUB_SUB:
                return {Role::PUBLISHER, Role::SUBSCRIBER};
            case Pattern::PUSH_PULL:
                return {Role::PUSHER, Role::PULLER};
            case Pattern::DEALER_ROUTER:
                return {Role::DEALER, Role::ROUTER};
        }
        return {};
    }

    explicit ZeroMQHandler(QObject* parent = nullptr);
    ~ZeroMQHandler() override;

    /**
     * @brief Configure ZeroMQ socket
     * @param pattern Socket pattern to use
     * @param role Role for DEALER-ROUTER pattern
     * @param endpoint ZeroMQ endpoint URL
     */
    void configure(Pattern pattern, Role role, std::string_view endpoint);

    /**
     * @brief Set socket options
     * @param option ZMQ option name
     * @param value Option value
     */
    template<typename T>
    void setOption(int option, const T& value) {
        if (m_socket) {
            m_socket->set(option, value);
        }
    }

    /**
     * @brief Subscribe to topic (PUB-SUB pattern)
     * @param topics Topics to subscribe to
     */
    void subscribe(const std::vector<std::string>& topics = {});

    /**
     * @brief Set socket timeout
     * @param timeout Timeout in milliseconds
     */
    void setTimeout(int timeout);

    ConnectionStatus getConnectionStatus() const { return m_status; }
    std::string getLastError() const { return m_lastError; }

    std::future<RequestResult> executeAsync(const RequestConfig& config) override;
    RequestResult execute(const RequestConfig& config) override;
    void cancel() override;

    void setConnectionStatus(ConnectionStatus status) {
        if (m_status != status) {
            m_status = status;
            emit connectionStatusChanged();
        }
    }

signals:
    /**
     * @brief Signal emitted when a message is received outside of a request/response cycle
     * @param message The received message content
     */
    void messageReceived(const QString& message);
    void errorOccurred(const QString& error);
    void connectionStatusChanged();

private:
    ConnectionStatus m_status{ConnectionStatus::DISCONNECTED};
    std::string m_lastError;

    void close();
    void configureSocket(Pattern pattern, Role role);
    void setupREQREP();
    void setupPUBSUB();
    void setupDEALERROUTER();
    void setCommonSocketOptions();
    void startPolling();
    void stopPolling();
    bool sendMessage(const std::string& message, bool more = false);
    
    // Message handling methods
    void handleREQREPMessage();
    void handlePUBSUBMessage();
    void handlePUSHPULLMessage();
    void handleDEALERROUTERMessage();
    void processIncomingMessage(const zmq::message_t& identity, zmq::message_t& message);
    
    static RequestResult createResult(const std::string& body, bool success = true) {
        RequestResult result;
        result.status_code = success ? 200 : 500;
        result.body = body;
        result.headers = {};
        result.metrics.bytes_received = body.size();
        return result;
    }
    
    // ZMQ context and socket
    zmq::context_t m_context{1};
    std::unique_ptr<zmq::socket_t> m_socket;
    
    // Configuration
    Pattern m_pattern{Pattern::REQ_REP};
    Role m_role{Role::REQUESTER};
    std::string m_endpoint;
    int m_timeout{500};
    
    // Thread management
    std::atomic<bool> m_running{false};
    std::unique_ptr<std::thread> m_pollThread;
    
    // Router/Dealer specific settings
    std::string m_dealerId;  // Unique identifier for DEALER socket
    std::string m_identity;  // Store last received identity for ROUTER responses
    
    // Router queues
    std::map<std::string, std::queue<std::string>> m_routerQueues;  // Message queues per client
    std::mutex m_routerQueuesMutex;
    
    struct QueuedMessage {
        std::string message;
        std::chrono::steady_clock::time_point timestamp;
        bool requiresResponse;
    };
    
    std::queue<QueuedMessage> m_outgoingQueue;
    std::mutex m_queueMutex;
    
    // Performance metrics
    struct Metrics {
        std::atomic<uint64_t> messagesReceived{0};
        std::atomic<uint64_t> messagesSent{0};
        std::atomic<uint64_t> bytesReceived{0};
        std::atomic<uint64_t> bytesSent{0};
    } m_metrics;
};

} // namespace flowdriver 
