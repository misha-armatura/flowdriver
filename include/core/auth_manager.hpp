#pragma once

#include "types.hpp"
#include <string_view>
#include <memory>
#include <functional>

namespace flowdriver {

class AuthManager {
public:
    using TokenRefreshCallback = std::function<std::string()>;

    AuthManager();
    ~AuthManager();

    /**
     * @brief Configure basic authentication
     * @param username Username
     * @param password Password
     */
    void setBasicAuth(std::string_view username, std::string_view password);

    /**
     * @brief Configure bearer token authentication
     * @param token Bearer token
     * @param refresh_callback Optional callback for token refresh
     */
    void setBearerToken(std::string_view token, 
                       TokenRefreshCallback refresh_callback = nullptr);

    /**
     * @brief Configure API key authentication
     * @param name API key name/header
     * @param value API key value
     * @param in_header True if key should be sent in header, false for query param
     */
    void setApiKey(std::string_view name, std::string_view value, bool in_header = true);

    /**
     * @brief Apply authentication to request headers
     * @param headers Headers to modify
     */
    void applyAuth(std::vector<Header>& headers);

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace flowdriver 
