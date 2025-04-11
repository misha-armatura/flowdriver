#pragma once

#include "protocol_handler.hpp"
#include <QWebSocket>
#include <QUrl>
#include <QNetworkRequest>
#include "core/error.hpp"
#include <boost/beast/websocket.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/url.hpp>
#include <boost/url/url_view_base.hpp>
#include <boost/url/url.hpp>
#include <boost/url/parse.hpp>
#include <functional>
#include <string>

namespace flowdriver {

class WebSocketHandler : public ProtocolHandler {
    Q_OBJECT

public:
    using MessageCallback = std::function<void(std::string_view)>;
    using ErrorCallback = std::function<void(const Error&)>;

    explicit WebSocketHandler(QObject* parent = nullptr);
    ~WebSocketHandler() override;

    /**
     * @brief Connect to WebSocket server
     * @param config Request configuration containing URL and headers
     */
    void connect(const RequestConfig& config);

    /**
     * @brief Send message asynchronously
     * @param config Request configuration
     */
    std::future<RequestResult> executeAsync(const RequestConfig& config) override;
    
    /**
     * @brief Send message synchronously
     * @param config Request configuration
     */
    RequestResult execute(const RequestConfig& config) override;

    /**
     * @brief Close the WebSocket connection
     */
    void cancel() override;

    /**
     * @brief Set message handler callback
     * @param callback Function to handle incoming messages
     */
    void setMessageHandler(MessageCallback callback);

    /**
     * @brief Set error handler callback
     * @param callback Function to handle errors
     */
    void setErrorHandler(ErrorCallback callback);

signals:
    void connected();
    void disconnected();
    void messageReceived(const std::string& message);
    void errorOccurred(const QString& message);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace flowdriver 
