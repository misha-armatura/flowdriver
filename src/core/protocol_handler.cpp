#include "core/protocol_handler.hpp"
#include "core/error.hpp"
#include <stdexcept>

namespace flowdriver {

void ProtocolHandler::validateConfig(const RequestConfig& config) {
    if (config.url.empty()) {
        throw Error(ErrorCode::INVALID_CONFIG, "URL cannot be empty");
    }
    if (config.method.empty()) {
        throw Error(ErrorCode::INVALID_CONFIG, "Method cannot be empty");
    }
}

RequestResult ProtocolHandler::execute(const RequestConfig& config) {
    auto future = executeAsync(config);
    return future.get();
}

} // namespace flowdriver 