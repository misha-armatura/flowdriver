#pragma once

#include <string>
#include <system_error>

namespace flowdriver {

enum class ErrorCode {
    NONE,
    INVALID_ARGUMENT,
    INTERNAL_ERROR,
    INVALID_STATE,
    NETWORK_ERROR,
    SSL_ERROR,
    TIMEOUT,
    PARSE_ERROR,
    INVALID_CONFIG,
    UNKNOWN,
    PROTOCOL_ERROR,
    ZMQ_ERROR
};

class Error : public std::runtime_error {
public:
    Error(ErrorCode code, const std::string& message)
        : std::runtime_error(message), m_code(code) {}
    
    ErrorCode code() const { return m_code; }
    
private:
    ErrorCode m_code;
};

} // namespace flowdriver 
