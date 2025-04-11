#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <variant>

namespace flowdriver {

/**
 * @brief Common types used across the application
 */
struct Header {
    std::string name;
    std::string value;
};

enum class Protocol {
    REST,
    WEBSOCKET,
    GRPC,
    ZEROMQ
};

enum class AuthType {
    NONE,
    BASIC,
    BEARER,
    API_KEY
};

struct AuthConfig {
    AuthType type{AuthType::NONE};
    std::string username;
    std::string password;
    std::string token;
    std::string api_key;
    std::string api_key_location;
    std::string api_key_name;
};

struct RequestConfig {
    Protocol protocol{Protocol::REST};
    std::string method;
    std::string url;
    std::vector<Header> headers;
    std::string body;
    std::optional<AuthConfig> auth;
    std::chrono::milliseconds timeout{5000};
};

struct RequestMetrics {
    std::chrono::microseconds total_time{0};
    std::chrono::microseconds dns_time{0};
    std::chrono::microseconds connect_time{0};
    std::chrono::microseconds tls_time{0};
    std::chrono::microseconds first_byte_time{0};
    size_t bytes_sent{0};
    size_t bytes_received{0};
};

struct RequestResult {
    int status_code{0};
    std::vector<Header> headers;
    std::string body;
    RequestMetrics metrics;
    std::string error;
};

} // namespace flowdriver 