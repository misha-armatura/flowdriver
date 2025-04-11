#include "core/websocket_handler.hpp"
#include "core/error.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/url.hpp>
#include <boost/asio/buffer.hpp>
#include <optional>
#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = net::ssl;
namespace urls = boost::urls;

namespace flowdriver {

class WebSocketHandler::Impl {
public:
    Impl() 
        : ioc_()
        , ssl_ctx_(ssl::context::tlsv12_client)
        , resolver_(ioc_)
        , work_guard_(net::make_work_guard(ioc_))
        , io_thread_([this] { ioc_.run(); }) 
    {
        ssl_ctx_.set_verify_mode(ssl::verify_peer);
        ssl_ctx_.set_default_verify_paths();
    }

    ~Impl() {
        close();
        work_guard_.reset();
        if (io_thread_.joinable()) {
            io_thread_.join();
        }
    }

    void connect(const RequestConfig& config) {
        try {
            auto result = urls::parse_uri(config.url);
            if (!result) {
                throw Error(ErrorCode::INVALID_CONFIG, "Invalid WebSocket URL format");
            }

            auto parsed_url = *result;
            if (parsed_url.scheme() != "ws" && parsed_url.scheme() != "wss") {
                throw Error(ErrorCode::INVALID_CONFIG, "URL must start with ws:// or wss://");
            }

            bool is_secure = parsed_url.scheme() == "wss";
            std::string host = std::string(parsed_url.host());
            if (host.empty()) {
                throw Error(ErrorCode::INVALID_CONFIG, "Invalid host in WebSocket URL");
            }

            std::string port = parsed_url.has_port() ? 
                std::string(parsed_url.port()) : 
                (is_secure ? "443" : "80");
            std::string target = std::string(parsed_url.path());
            if (target.empty()) {
                target = "/";
            }

            auto const results = resolver_.resolve(host, port);
            if (results.empty()) {
                throw Error(ErrorCode::NETWORK_ERROR, "Failed to resolve host");
            }

            if (is_secure) {
                auto ws = std::make_unique<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>(ioc_, ssl_ctx_);
                
                // Set SNI hostname
                SSL_set_tlsext_host_name(ws->next_layer().native_handle(), host.c_str());
                
                // Connect
                beast::get_lowest_layer(*ws).connect(results);
                
                // SSL handshake
                ws->next_layer().handshake(ssl::stream_base::client);
                
                // WebSocket handshake
                ws->set_option(websocket::stream_base::decorator(
                    [&config](websocket::request_type& req) {
                        req.set(beast::http::field::user_agent, "FlowDriver WebSocket Client");
                        
                        // Add all headers from config
                        for (const auto& header : config.headers) {
                            req.set(header.name, header.value);
                        }
                    }));
                
                ws->handshake(host, target);
                ws_ = std::move(ws);
            } else {
                auto ws = std::make_unique<websocket::stream<beast::tcp_stream>>(ioc_);
                
                // Connect
                beast::get_lowest_layer(*ws).connect(results);
                
                // WebSocket handshake
                std::cout << "Attempting WebSocket handshake to: " << host << target << std::endl;
                ws->set_option(websocket::stream_base::decorator(
                    [&config](websocket::request_type& req) {
                        req.set(beast::http::field::user_agent, "FlowDriver WebSocket Client");
                        
                        // Add headers from config (if any)
                        for (const auto& header : config.headers) {
                            req.set(header.name, header.value);
                        }

                        std::cout << "Handshake request headers:" << std::endl;
                        for(auto const& field : req.base()) {
                            std::cout << field.name_string() << ": " << field.value() << std::endl;
                        }
                    }));
                
                beast::error_code ec;
                ws->handshake(host, target, ec);
                if (ec) {
                    std::string error_msg = std::string("WebSocket handshake failed: ") + ec.message();
                    std::cout << "Handshake error: " << error_msg << std::endl;
                    throw Error(ErrorCode::NETWORK_ERROR, error_msg);
                }
                
                std::cout << "WebSocket handshake successful" << std::endl;
                ws_ = std::move(ws);
            }

            // Start reading
            doRead();
            
            // Emit connected signal through the callback
            if (connected_callback_) {
                connected_callback_();
            }

        } catch (const boost::system::system_error& e) {
            throw Error(ErrorCode::NETWORK_ERROR, std::string("Connection failed: ") + e.what());
        } catch (const Error& e) {
            throw;
        } catch (const std::exception& e) {
            throw Error(ErrorCode::UNKNOWN, std::string("Unexpected error: ") + e.what());
        }
    }

    RequestResult execute(const RequestConfig& config) {
        if (ws_.valueless_by_exception() || !std::visit([](const auto& ws) { return ws != nullptr; }, ws_)) {
            throw Error(ErrorCode::NETWORK_ERROR, "WebSocket not connected");
        }

        try {
            std::visit([&config](auto& ws) {
                if (!config.body.empty()) {
                    ws->write(boost::asio::buffer(config.body));
                } else {
                    ws->write(boost::asio::buffer("")); // Send empty string if no body
                }
            }, ws_);

            beast::flat_buffer buffer;
            std::visit([&buffer](auto& ws) {
                ws->read(buffer);
            }, ws_);

            RequestResult result;
            result.status_code = 200;
            result.body = beast::buffers_to_string(buffer.data());
            return result;
        } catch (const boost::system::system_error& e) {
            throw Error(ErrorCode::NETWORK_ERROR, e.what());
        }
    }

    std::future<RequestResult> executeAsync(const RequestConfig& config) {
        return std::async(std::launch::async, [this, config]() {
            return execute(config);
        });
    }

    void close() {
        if (!ws_.valueless_by_exception()) {
            boost::system::error_code ec;
            std::visit([&ec](auto& ws) {
                if (ws) {
                    ws->close(websocket::close_code::normal, ec);
                }
            }, ws_);
        }
    }

    void setMessageHandler(MessageCallback callback) {
        message_callback_ = std::move(callback);
    }

    void setErrorHandler(ErrorCallback callback) {
        error_callback_ = std::move(callback);
    }

    void setConnectedCallback(std::function<void()> callback) {
        connected_callback_ = std::move(callback);
    }

private:
    void doRead() {
        std::visit([this](auto& ws) {
            if (ws) {
                ws->async_read(
                    read_buffer_,
                    [this](beast::error_code ec, std::size_t bytes_transferred) {
                        if (!ec) {
                            if (message_callback_) {
                                std::string_view message(
                                    static_cast<char*>(read_buffer_.data().data()),
                                    bytes_transferred);
                                message_callback_(message);
                            }
                            
                            read_buffer_.consume(bytes_transferred);
                            doRead();
                        } else if (error_callback_) {
                            error_callback_(Error(ErrorCode::NETWORK_ERROR, ec.message()));
                        }
                    });
            }
        }, ws_);
    }

    net::io_context ioc_;
    ssl::context ssl_ctx_;
    net::ip::tcp::resolver resolver_;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;
    std::thread io_thread_;
    
    std::variant<
        std::unique_ptr<websocket::stream<beast::tcp_stream>>,
        std::unique_ptr<websocket::stream<beast::ssl_stream<beast::tcp_stream>>>
    > ws_;
    
    beast::flat_buffer read_buffer_;
    MessageCallback message_callback_;
    ErrorCallback error_callback_;
    std::function<void()> connected_callback_;
};

WebSocketHandler::WebSocketHandler(QObject* parent) 
    : ProtocolHandler(parent)
    , pimpl_(std::make_unique<Impl>()) 
{
    // Set up message handler to emit signal
    pimpl_->setMessageHandler([this](std::string_view msg) {
        emit messageReceived(std::string(msg));
    });

    // Set up connected handler
    pimpl_->setConnectedCallback([this]() {
        emit connected();
    });
}

WebSocketHandler::~WebSocketHandler() = default;

void WebSocketHandler::connect(const RequestConfig& config) {
    try {
        pimpl_->connect(config);
    } catch (const Error& e) {
        // Log the error but don't rethrow
        qDebug() << "WebSocket connection error:" << QString::fromStdString(e.what());
        emit errorOccurred(QString::fromStdString(e.what()));
    } catch (const std::exception& e) {
        qDebug() << "Unexpected WebSocket error:" << e.what();
        emit errorOccurred(QString("Unexpected error: %1").arg(e.what()));
    }
}

RequestResult WebSocketHandler::execute(const RequestConfig& config) {
    return pimpl_->execute(config);
}

std::future<RequestResult> WebSocketHandler::executeAsync(const RequestConfig& config) {
    return pimpl_->executeAsync(config);
}

void WebSocketHandler::cancel() {
    pimpl_->close();
}

void WebSocketHandler::setMessageHandler(MessageCallback callback) {
    pimpl_->setMessageHandler(std::move(callback));
}

void WebSocketHandler::setErrorHandler(ErrorCallback callback) {
    pimpl_->setErrorHandler(std::move(callback));
}

} // namespace flowdriver 
