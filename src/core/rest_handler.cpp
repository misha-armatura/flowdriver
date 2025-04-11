#include "core/rest_handler.hpp"
#include "core/error.hpp"
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <variant>
#include <chrono>
#include <QDebug>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;

namespace flowdriver {

using ssl_stream_t = beast::ssl_stream<beast::tcp_stream>;
using stream_variant_t = std::variant<beast::tcp_stream*, ssl_stream_t*>;

// Add this helper function before the RestHandler class implementation
std::tuple<std::string, std::string, std::string> parseUrl(const std::string& url) {
    std::string host;
    std::string port = "80"; // Default HTTP port
    std::string target = "/";

    // Remove protocol if present
    size_t start = 0;
    if (url.substr(0, 7) == "http://") {
        start = 7;
    } else if (url.substr(0, 8) == "https://") {
        start = 8;
        port = "443"; // Default HTTPS port
    }

    // Find the first slash after the host[:port]
    size_t pathStart = url.find('/', start);
    
    // Extract host[:port]
    std::string hostPort = url.substr(start, pathStart == std::string::npos ? std::string::npos : pathStart - start);
    
    // Split host and port if port is specified
    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos) {
        host = hostPort.substr(0, colonPos);
        port = hostPort.substr(colonPos + 1);
    } else {
        host = hostPort;
    }

    // Extract target path
    if (pathStart != std::string::npos) {
        target = url.substr(pathStart);
    }

    return {host, port, target};
}

namespace {
    void log_ssl_errors() {
        unsigned long err = ERR_get_error();
        while (err) {
            char buf[256];
            // ERR_error_string_long(err, buf);
            qDebug() << "SSL Error:" << buf;
            err = ERR_get_error();
        }
    }

    void dump_cert_info(SSL* ssl) {
        X509* cert = SSL_get_peer_certificate(ssl);
        if (cert) {
            qDebug() << "Server certificate:";
            char* subject = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
            char* issuer = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
            qDebug() << "  Subject:" << subject;
            qDebug() << "  Issuer:" << issuer;
            OPENSSL_free(subject);
            OPENSSL_free(issuer);
            X509_free(cert);
        } else {
            qDebug() << "No certificate provided by peer";
        }
    }
}

class RestHandler::Impl {
    friend class RestHandler;
public:
    Impl() 
        : ioc_()
        , work_guard_(net::make_work_guard(ioc_)) {
        
        qDebug() << "Initializing SSL context...";
        
        // Create SSL context with system certificates
        ssl_ctx_ = std::make_shared<ssl::context>(ssl::context::tlsv12_client);
        ssl_ctx_->set_default_verify_paths();
        ssl_ctx_->set_verify_mode(ssl::verify_peer);
        
        qDebug() << "SSL context initialized";
    }

    std::future<RequestResult> executeAsync(const RequestConfig& config) {
        return std::async(std::launch::async, [this, config]() {
            try {
                qDebug() << "Executing request:" << QString::fromStdString(config.url);

                auto [host, port, target] = parseUrl(config.url);
                bool use_ssl = config.url.substr(0, 8) == "https://";  // Исправил проверку HTTPS

                qDebug() << "Parsed URL - Host:" << QString::fromStdString(host)
                         << "Port:" << QString::fromStdString(port)
                         << "Target:" << QString::fromStdString(target)
                         << "SSL:" << use_ssl;

                          // Create request
                http::request<http::string_body> req{
                    http::string_to_verb(config.method),
                    target,
                    11  // HTTP/1.1
                };

                req.set(http::field::host, host);
                req.set(http::field::user_agent, "FlowDriver/1.0");

                          // Add headers
                for (const auto& header : config.headers) {
                    req.set(header.name, header.value);
                }

                          // Add body if present
                if (!config.body.empty()) {
                    req.body() = config.body;
                    req.prepare_payload();
                }

                          // Create stream
                beast::tcp_stream base_stream(ioc_);
                std::unique_ptr<beast::ssl_stream<beast::tcp_stream>> ssl_stream;

                          // Resolve endpoint
                qDebug() << "Resolving hostname...";
                auto resolver = net::ip::tcp::resolver(ioc_);
                auto const endpoints = resolver.resolve(host, port);
                qDebug() << "Resolved" << endpoints.size() << "endpoints";

                          // Buffer for response
                beast::flat_buffer buffer;
                http::response<http::string_body> res;

                if (use_ssl) {
                    qDebug() << "Setting up SSL stream...";
                    ssl_stream = std::make_unique<beast::ssl_stream<beast::tcp_stream>>(
                        std::move(base_stream), *ssl_ctx_);

                    qDebug() << "Setting SNI hostname:" << QString::fromStdString(host);
                    if (!SSL_set_tlsext_host_name(ssl_stream->native_handle(), host.c_str())) {
                        throw beast::system_error(
                            beast::error_code(
                                static_cast<int>(::ERR_get_error()),
                                net::error::get_ssl_category()),
                            "Failed to set SNI Hostname");
                    }

                    qDebug() << "Connecting to endpoint...";
                    beast::get_lowest_layer(*ssl_stream).connect(endpoints);

                    qDebug() << "Starting SSL handshake...";
                    ssl_stream->handshake(ssl::stream_base::client);
                    qDebug() << "SSL handshake completed successfully";

                    // Dump certificate info
                    dump_cert_info(ssl_stream->native_handle());

                    qDebug() << "Writing request...";
                    http::write(*ssl_stream, req);

                    qDebug() << "Reading response...";
                    http::read(*ssl_stream, buffer, res);

                    qDebug() << "Starting SSL shutdown...";
                    beast::error_code ec;
                    ssl_stream->shutdown(ec);

                    if(ec &&
                         ec != beast::errc::not_connected &&
                         ec != net::error::eof &&
                         ec.category() != net::ssl::error::get_stream_category()) {
                        qDebug() << "SSL shutdown unexpected error:" << ec.message().c_str();
                    } else {
                        qDebug() << "SSL connection closed cleanly";
                    }
                } else {
                    qDebug() << "Connecting non-SSL stream...";
                    base_stream.connect(endpoints);

                    qDebug() << "Writing request...";
                    http::write(base_stream, req);

                    qDebug() << "Reading response...";
                    http::read(base_stream, buffer, res);
                }

                          // Prepare result
                RequestResult result;
                result.status_code = res.result_int();
                result.body = res.body();

                for (const auto& header : res) {
                    result.headers.push_back({
                        std::string(header.name_string()),
                        std::string(header.value())
                    });
                }

                return result;

            } catch (const beast::system_error& e) {
                qDebug() << "Beast error:" << e.what();
                throw Error(ErrorCode::NETWORK_ERROR, e.what());
            } catch (const std::exception& e) {
                qDebug() << "General error:" << e.what();
                throw Error(ErrorCode::UNKNOWN, e.what());
            }
        });
    }

    void setSSLContext(std::shared_ptr<ssl::context> ctx) {
        ssl_ctx_ = ctx;
    }

private:
    net::io_context ioc_;
    std::shared_ptr<ssl::context> ssl_ctx_;
    net::executor_work_guard<net::io_context::executor_type> work_guard_;
};

// Implementation of public interface
RestHandler::RestHandler() : pimpl_(std::make_unique<Impl>()) {}
RestHandler::~RestHandler() = default;

std::future<RequestResult> RestHandler::executeAsync(const RequestConfig& config) {
    return pimpl_->executeAsync(config);
}

RequestResult RestHandler::execute(const RequestConfig& config) {
    return executeAsync(config).get();
}

void RestHandler::setSSLContext(std::shared_ptr<ssl::context> ctx) {
    if (pimpl_) {
        pimpl_->setSSLContext(ctx);
    }
}

void RestHandler::cancel() {
    // Implement if needed
}

} // namespace flowdriver 
