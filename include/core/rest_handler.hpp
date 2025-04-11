#pragma once

#include "core/protocol_handler.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <concepts>
#include <span>
#include <memory>
#include <functional>

namespace flowdriver {

// Helper function to parse URLs
std::tuple<std::string, std::string, std::string> parseUrl(const std::string& url);

class RestHandler final : public ProtocolHandler {
public:
    // Add SSL verification callback type
    using VerifyCallback = std::function<bool(bool, boost::asio::ssl::verify_context&)>;
    
    RestHandler();
    ~RestHandler() override;

    // Add method to set custom verification
    void setSSLVerifyCallback(VerifyCallback callback);

    std::future<RequestResult> executeAsync(const RequestConfig& config) override;
    RequestResult execute(const RequestConfig& config) override;
    void cancel() override;

    // SSL configuration using Beast's SSL context
    void setSSLContext(std::shared_ptr<boost::asio::ssl::context> ctx);
    
    // Add connection pooling
    void setMaxConnections(size_t max_connections);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace flowdriver 
