#include "core/zeromq_handler.hpp"
#include "core/error.hpp"
#include <thread>
#include <future>
#include <chrono>
#include <random>
#include <QDebug>
#include <QObject>
#include <nlohmann/json.hpp>

namespace flowdriver {

// Helper function to convert role to string for logging
QString roleToString(ZeroMQHandler::Role role) {
    switch (role) {
    case ZeroMQHandler::Role::PUBLISHER: return "PUB";
    case ZeroMQHandler::Role::SUBSCRIBER: return "SUB";
    case ZeroMQHandler::Role::PUSHER: return "PUSH";
    case ZeroMQHandler::Role::PULLER: return "PULL";
    case ZeroMQHandler::Role::REQUESTER: return "REQ";
    case ZeroMQHandler::Role::REPLIER: return "REP";
    case ZeroMQHandler::Role::DEALER: return "DEALER";
    case ZeroMQHandler::Role::ROUTER: return "ROUTER";
    default: return "UNKNOWN";
    }
}

// Constructor / Destructor
ZeroMQHandler::ZeroMQHandler(QObject* parent) 
    : ProtocolHandler(parent)
    , m_context(1)
    , m_socket(nullptr)
    , m_pattern(Pattern::REQ_REP)
    , m_role(Role::REQUESTER)
{
}

ZeroMQHandler::~ZeroMQHandler() {
    ZeroMQHandler::close();
}

void ZeroMQHandler::close() {
    // Stop polling thread first
    stopPolling();
    
    if (m_socket) {
        try {
            // Set linger to 0 to prevent hanging
            m_socket->set(zmq::sockopt::linger, 0);
            
            // Unbind/disconnect based on role
            if (m_role == Role::PUBLISHER || 
                m_role == Role::PULLER ||
                m_role == Role::REPLIER || 
                m_role == Role::ROUTER) {
                try {
                    m_socket->unbind(m_endpoint);
                    qDebug() << "Successfully unbound from" << QString::fromStdString(m_endpoint);
                } catch (const zmq::error_t& e) {
                    qDebug() << "Unbind error (expected):" << e.what();
                }
            } else {
                try {
                    m_socket->disconnect(m_endpoint);
                    qDebug() << "Successfully disconnected from" << QString::fromStdString(m_endpoint);
                } catch (const zmq::error_t& e) {
                    qDebug() << "Disconnect error (expected):" << e.what();
                }
            }
            
            m_socket->close();
            qDebug() << "Socket closed";
        } catch (const std::exception& e) {
            qDebug() << "Error closing socket:" << e.what();
        }
        m_socket.reset();
    }
    
    // Wait longer for the OS to release the port
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    qDebug() << "ZMQ handler closed completely";
}

void ZeroMQHandler::configureSocket(Pattern pattern, Role role) {
    switch (pattern) {
        case Pattern::PUSH_PULL:
            if (role == Role::PUSHER) {
                m_socket = std::make_unique<zmq::socket_t>(m_context, zmq::socket_type::push);
                qDebug() << "Created PUSH socket";
            } else if (role == Role::PULLER) {
                m_socket = std::make_unique<zmq::socket_t>(m_context, zmq::socket_type::pull);
                qDebug() << "Created PULL socket";
            }
            break;
        case Pattern::REQ_REP:
            setupREQREP();
            break;
        case Pattern::PUB_SUB:
            setupPUBSUB();
            break;
        case Pattern::DEALER_ROUTER:
            setupDEALERROUTER();
            break;
    }
}

void ZeroMQHandler::configure(Pattern pattern, Role role, std::string_view endpoint) {
    qDebug() << "ZMQ Configure - Pattern:" << static_cast<int>(pattern) 
             << "Role:" << static_cast<int>(role) 
             << "Endpoint:" << QString::fromStdString(std::string(endpoint));

    close();
    
    setConnectionStatus(ConnectionStatus::DISCONNECTED);
    m_pattern = pattern;
    m_role = role;
    m_endpoint = std::string(endpoint);
    
    try {
        configureSocket(pattern, role);
        
        setCommonSocketOptions();
        
        std::string bindEndpoint = m_endpoint;
        if (m_role == Role::PUBLISHER || 
            m_role == Role::PULLER ||
            m_role == Role::REPLIER || 
            m_role == Role::ROUTER) {
            size_t pos = bindEndpoint.find("//");
            if (pos != std::string::npos) {
                pos += 2;
                size_t colonPos = bindEndpoint.find(":", pos);
                if (colonPos != std::string::npos) {
                    bindEndpoint = bindEndpoint.substr(0, pos) + "*" + bindEndpoint.substr(colonPos);
                }
            }
            
            qDebug() << "Binding" << roleToString(m_role) << "socket to:" << QString::fromStdString(bindEndpoint);
            try {
                m_socket->bind(bindEndpoint);
                qDebug() << "Bind successful";
            } catch (const zmq::error_t& e) {
                qDebug() << "Bind error:" << e.what();
                throw Error(ErrorCode::ZMQ_ERROR, std::string("Failed to bind: ") + e.what());
            }
        } else {
            qDebug() << "Connecting" << roleToString(m_role) << "socket to:" << QString::fromStdString(m_endpoint);
            try {
                m_socket->connect(m_endpoint);
                qDebug() << "Connect successful";
            } catch (const zmq::error_t& e) {
                qDebug() << "Connect error:" << e.what();
                throw Error(ErrorCode::ZMQ_ERROR, std::string("Failed to connect: ") + e.what());
            }
        }

        // Start polling thread
        startPolling();
        
        setConnectionStatus(ConnectionStatus::CONNECTED);
        qDebug() << "ZMQ Socket configured and connected successfully";
        
    } catch (const zmq::error_t& e) {
        qDebug() << "ZMQ error during configure:" << e.what();
        setConnectionStatus(ConnectionStatus::ERROR);
        throw Error(ErrorCode::ZMQ_ERROR, e.what());
    }
}

void ZeroMQHandler::setupDEALERROUTER() {
    if (m_role == Role::DEALER) {
        m_socket = std::make_unique<zmq::socket_t>(m_context, zmq::socket_type::dealer);
        
        // Generate unique dealer ID using a proper random device
        std::random_device rd;
        std::mt19937 gen(rd());
        m_dealerId = "DEALER-" + std::to_string(gen());
        m_identity = m_dealerId; // Set identity for both variables
        
        // Set socket options
        m_socket->set(zmq::sockopt::routing_id, m_dealerId);
        m_socket->set(zmq::sockopt::linger, 0);
        m_socket->set(zmq::sockopt::reconnect_ivl, 1000);
        m_socket->set(zmq::sockopt::rcvtimeo, m_timeout);
        m_socket->set(zmq::sockopt::sndtimeo, m_timeout);
        
        qDebug() << "DEALER socket created with ID:" << QString::fromStdString(m_dealerId);
        qDebug() << "DEALER connecting to:" << QString::fromStdString(m_endpoint);
        
    } else if (m_role == Role::ROUTER) {
        m_socket = std::make_unique<zmq::socket_t>(m_context, zmq::socket_type::router);
        
        // Set socket options
        m_socket->set(zmq::sockopt::linger, 0);
        m_socket->set(zmq::sockopt::rcvtimeo, m_timeout);
        m_socket->set(zmq::sockopt::sndtimeo, m_timeout);
        
        // For binding roles, use wildcard address
        std::string bindEndpoint = m_endpoint;
        size_t pos = bindEndpoint.find("//");
        if (pos != std::string::npos) {
            pos += 2;
            size_t colonPos = bindEndpoint.find(":", pos);
            if (colonPos != std::string::npos) {
                bindEndpoint = bindEndpoint.substr(0, pos) + "*" + bindEndpoint.substr(colonPos);
            }
        }
        
        // Set a default identity for ROUTER
        std::random_device rd;
        std::mt19937 gen(rd());
        m_identity = "Client";
        
        qDebug() << "ROUTER socket created, binding to:" << QString::fromStdString(bindEndpoint);
    }
}

void ZeroMQHandler::setupREQREP() {
    qDebug() << "Setting up REQ-REP pattern";
    if (m_role == Role::REQUESTER) {
        qDebug() << "Creating REQ socket";
        m_socket = std::make_unique<zmq::socket_t>(m_context, zmq::socket_type::req);
    } else if (m_role == Role::REPLIER) {
        qDebug() << "Creating REP socket";
        m_socket = std::make_unique<zmq::socket_t>(m_context, zmq::socket_type::rep);
    }
}

void ZeroMQHandler::setupPUBSUB() {
    if (m_role == Role::PUBLISHER) {
        m_socket = std::make_unique<zmq::socket_t>(m_context, zmq::socket_type::pub);
        setCommonSocketOptions();
        // Add a small delay to allow subscribers to connect
        m_socket->set(zmq::sockopt::linger, 1000);
        qDebug() << "PUB socket created";
    } else if (m_role == Role::SUBSCRIBER) {
        m_socket = std::make_unique<zmq::socket_t>(m_context, zmq::socket_type::sub);
        setCommonSocketOptions();
        // Subscribe to all messages
        m_socket->set(zmq::sockopt::subscribe, "");
        // Increase receive timeout for subscribers
        m_socket->set(zmq::sockopt::rcvtimeo, 5000);
        qDebug() << "SUB socket created";
        // Add a small delay to ensure subscription is established
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Add this helper function to set common socket options
void ZeroMQHandler::setCommonSocketOptions() {
    if (!m_socket) return;
    
    // Increase timeouts for REQ-REP pattern
    if (m_pattern == Pattern::REQ_REP) {
        m_timeout = 30000;
    }
    
    // Common socket options
    m_socket->set(zmq::sockopt::rcvtimeo, m_timeout);
    m_socket->set(zmq::sockopt::sndtimeo, m_timeout);
    m_socket->set(zmq::sockopt::immediate, 1);
    m_socket->set(zmq::sockopt::rcvhwm, 1000);
    m_socket->set(zmq::sockopt::sndhwm, 1000);
    m_socket->set(zmq::sockopt::linger, 0);     // Don't wait on close
    m_socket->set(zmq::sockopt::reconnect_ivl, 100);  // Fast reconnect
    
    qDebug() << "Common socket options set with timeout:" << m_timeout << "ms";
}

void ZeroMQHandler::setTimeout(int timeout) {
    m_timeout = timeout;
    if (m_socket) {
        m_socket->set(zmq::sockopt::rcvtimeo, m_timeout);
        m_socket->set(zmq::sockopt::sndtimeo, m_timeout);
    }
}

void ZeroMQHandler::subscribe(const std::vector<std::string>& topics) {
    if (!m_socket || m_role != Role::SUBSCRIBER) {
        return;
    }
    
    if (topics.empty()) {
        m_socket->set(zmq::sockopt::subscribe, "");
    } else {
        for (const auto& topic : topics) {
            m_socket->set(zmq::sockopt::subscribe, topic);
        }
    }
}

RequestResult ZeroMQHandler::execute(const RequestConfig& config) {
    if (!m_socket) {
        throw Error(ErrorCode::ZMQ_ERROR, "Socket not initialized");
    }
    
    try {
        // Prevent SUBSCRIBER from sending messages
        if (m_role == Role::SUBSCRIBER) {
            throw Error(ErrorCode::ZMQ_ERROR, "Subscribers cannot send messages");
        }

        qDebug() << "Executing ZMQ request with body:" << QString::fromStdString(config.body);
        
        if (m_role == Role::PUBLISHER) {
            zmq::message_t message(config.body.data(), config.body.size());
            if (!m_socket->send(message, zmq::send_flags::none)) {
                throw Error(ErrorCode::ZMQ_ERROR, "Failed to publish message");
            }
            emit messageReceived(QString::fromStdString(config.body));
            return createResult(config.body);
        }
        
        if (m_role == Role::DEALER) {
            // For DEALER, just send the message directly
            zmq::message_t message(config.body.data(), config.body.size());
            auto send_result = m_socket->send(message, zmq::send_flags::none);
            
            if (!send_result) {
                qDebug() << "Failed to send DEALER message";
                throw Error(ErrorCode::ZMQ_ERROR, "Failed to send message");
            }
            
            qDebug() << "DEALER message sent successfully";
            emit messageReceived(QString::fromStdString(config.body));
            
            // Wait for response from ROUTER
            zmq::message_t response;
            auto recv_result = m_socket->recv(response, zmq::recv_flags::none);
            
            if (recv_result) {
                std::string response_str(static_cast<char*>(response.data()), response.size());
                qDebug() << "DEALER received response:" << QString::fromStdString(response_str);
                return createResult(response_str);
            }
            
            return createResult(config.body);
        }
        
        if (m_role == Role::ROUTER) {
            // For ROUTER, we need to send identity frame first
            if (m_identity.empty()) {
                m_identity = "Game"; // Default identity if none is set
            }
            
            zmq::message_t identity(m_identity.data(), m_identity.size());
            if (!m_socket->send(identity, zmq::send_flags::sndmore)) {
                throw Error(ErrorCode::ZMQ_ERROR, "Failed to send identity frame");
            }
            
            // Then send the actual message
            zmq::message_t message(config.body.data(), config.body.size());
            if (!m_socket->send(message, zmq::send_flags::none)) {
                throw Error(ErrorCode::ZMQ_ERROR, "Failed to send message");
            }
            
            qDebug() << "ROUTER message sent successfully to" << QString::fromStdString(m_identity);
            emit messageReceived(QString::fromStdString(config.body));
            return createResult(config.body);
        }
        
        // Send the message for other patterns
        zmq::message_t request(config.body.data(), config.body.size());
        auto send_result = m_socket->send(request, zmq::send_flags::none);
        
        if (!send_result) {
            qDebug() << "Failed to send message";
            throw Error(ErrorCode::ZMQ_ERROR, "Failed to send message");
        }
        
        qDebug() << "Message sent successfully";
        emit messageReceived(QString::fromStdString(config.body));
        
        // For REQ-REP pattern, wait for response (only for REQUESTER)
        if (m_pattern == Pattern::REQ_REP && m_role == Role::REQUESTER) {
            zmq::message_t reply;
            auto recv_result = m_socket->recv(reply);
            
            if (recv_result) {
                std::string reply_str(static_cast<char*>(reply.data()), reply.size());
                qDebug() << "Received reply:" << QString::fromStdString(reply_str);
                return createResult(reply_str);
            } else {
                throw Error(ErrorCode::TIMEOUT, "No reply received");
            }
        }
        
        return createResult(config.body);
        
    } catch (const zmq::error_t& e) {
        qDebug() << "ZMQ error during execute:" << e.what();
        throw Error(ErrorCode::ZMQ_ERROR, e.what());
    }
}

std::future<RequestResult> ZeroMQHandler::executeAsync(const RequestConfig& config) {
    return std::async(std::launch::async, [this, config]() {
        return execute(config);
    });
}

void ZeroMQHandler::cancel() {
    close();
}

void ZeroMQHandler::startPolling() {
    stopPolling();
    
    if (!m_socket) {
        qDebug() << "Cannot start polling: socket not initialized";
        return;
    }
    
    m_running = true;
    m_pollThread = std::make_unique<std::thread>([this]() {
        qDebug() << "Poll thread started for role:" << static_cast<int>(m_role);
        
        while (m_running) {
            try {
                if (!m_socket) break;

                zmq::pollitem_t items[] = {
                    { m_socket->handle(), 0, ZMQ_POLLIN, 0 }
                };

                if (zmq::poll(items, 1, std::chrono::duration<long long, std::milli>(100)) > 0) {
                    if (items[0].revents & ZMQ_POLLIN) {
                        switch (m_pattern) {
                            case Pattern::REQ_REP:
                                handleREQREPMessage();
                                break;
                            case Pattern::PUB_SUB:
                                handlePUBSUBMessage();
                                break;
                            case Pattern::PUSH_PULL:
                                handlePUSHPULLMessage();
                                break;
                            case Pattern::DEALER_ROUTER:
                                handleDEALERROUTERMessage();
                                break;
                        }
                    }
                }
            } catch (const zmq::error_t& e) {
                qDebug() << "Error in poll thread:" << e.what();
                emit errorOccurred(QString::fromStdString(e.what()));
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
}

void ZeroMQHandler::handleREQREPMessage() {
    if (m_role == Role::REPLIER) {
        zmq::message_t message;
        if (m_socket->recv(message)) {
            std::string msg_str(static_cast<char*>(message.data()), message.size());
            emit messageReceived(QString::fromStdString(msg_str));
            
            // Send reply
            std::string reply = "Reply to: " + msg_str;
            zmq::message_t reply_msg(reply.data(), reply.size());
            if (m_socket->send(reply_msg, zmq::send_flags::none)) {
                qDebug() << "REPLIER sent reply";
            }
        }
    }
}

void ZeroMQHandler::handlePUBSUBMessage() {
    if (m_role == Role::SUBSCRIBER) {
        zmq::message_t message;
        if (m_socket->recv(message)) {
            std::string msg_str(static_cast<char*>(message.data()), message.size());
            QString qmsg = QString::fromStdString(msg_str);
            
            qDebug() << "SUB about to emit message:" << qmsg;
            emit messageReceived(qmsg);
            qDebug() << "SUB emitted message:" << qmsg;
        }
    }
}

void ZeroMQHandler::handlePUSHPULLMessage() {
    if (m_role == Role::PULLER) {
        zmq::message_t message;
        if (m_socket->recv(message)) {
            processIncomingMessage(zmq::message_t(), message);
        }
    }
}

void ZeroMQHandler::handleDEALERROUTERMessage() {
    std::vector<zmq::message_t> messages;
    bool more = true;
    
    try {
        while (more) {
            zmq::message_t message;
            auto recv_result = m_socket->recv(message, zmq::recv_flags::dontwait);
            if (recv_result) {
                messages.push_back(std::move(message));
                more = m_socket->get(zmq::sockopt::rcvmore);
            } else {
                break;
            }
        }
        
        if (messages.empty()) return;
        
        if (m_role == Role::ROUTER) {
            // First frame is identity for ROUTER
            if (messages.size() >= 2) {
                std::string identityStr(static_cast<const char*>(messages[0].data()), messages[0].size());
                std::string messageStr(static_cast<const char*>(messages[1].data()), messages[1].size());
                
                qDebug() << "ROUTER received message from client:" << QString::fromStdString(identityStr)
                         << "content:" << QString::fromStdString(messageStr);
                
                // Store the client identity for sending responses back
                m_identity = identityStr;
                
                // Process the message content
                emit messageReceived(QString::fromStdString(messageStr));
                
                // Send an immediate response back to the client
                if (!m_identity.empty()) {
                    std::string responseMsg = "Response to: " + messageStr;
                    zmq::message_t identity(m_identity.data(), m_identity.size());
                    zmq::message_t response(responseMsg.data(), responseMsg.size());
                    
                    m_socket->send(identity, zmq::send_flags::sndmore);
                    m_socket->send(response, zmq::send_flags::none);
                    
                    qDebug() << "ROUTER sent response to" << QString::fromStdString(m_identity)
                             << ":" << QString::fromStdString(responseMsg);
                }
            }
        } else if (m_role == Role::DEALER) {
            if (!messages.empty()) {
                std::string messageStr(static_cast<const char*>(messages[0].data()), messages[0].size());
                qDebug() << "DEALER received message:" << QString::fromStdString(messageStr);
                emit messageReceived(QString::fromStdString(messageStr));
            }
        }
    } catch (const zmq::error_t& e) {
        if (e.num() != EAGAIN) {  // Ignore would-block errors
            qDebug() << "Error in DEALER-ROUTER message handling:" << e.what();
            emit errorOccurred(QString::fromStdString(e.what()));
        }
    }
}

void ZeroMQHandler::processIncomingMessage(const zmq::message_t& identity, zmq::message_t& message) {
    try {
        std::string messageStr(static_cast<char*>(message.data()), message.size());
        
        // Update metrics
        m_metrics.messagesReceived++;
        m_metrics.bytesReceived += messageStr.size();
        
        // For ROUTER role, handle identity
        if (m_role == Role::ROUTER && !identity.empty()) {
            std::string identityStr(static_cast<const char*>(identity.data()), identity.size());
            m_identity = identityStr;
            qDebug() << "ROUTER received message from client:" << QString::fromStdString(identityStr);
            
            std::lock_guard<std::mutex> lock(m_routerQueuesMutex);
            m_routerQueues[identityStr].push(messageStr);
        }
        
        // Emit messages for all patterns except REQ-REP
        if (m_pattern != Pattern::REQ_REP) {
            emit messageReceived(QString::fromStdString(messageStr));
        }
        
    } catch (const std::exception& e) {
        emit errorOccurred(QString::fromStdString(e.what()));
    }
}

bool ZeroMQHandler::sendMessage(const std::string& message, bool more) {
    if (!m_socket) return false;

    try {
        zmq::message_t msg(message.data(), message.size());
        zmq::send_flags flags = more ? zmq::send_flags::sndmore : zmq::send_flags::none;
        return m_socket->send(msg, flags).has_value();
    } catch (const zmq::error_t& e) {
        qDebug() << "ZMQ send error:" << e.what();
        return false;
    }
}

void ZeroMQHandler::stopPolling() {
    m_running = false;
    
    if (m_pollThread && m_pollThread->joinable()) {
        try {
            m_pollThread->join();
        } catch (const std::exception& e) {
            qDebug() << "Error joining poll thread:" << e.what();
        }
    }
    m_pollThread.reset();
}

} // namespace flowdriver
