#pragma once

#include "core/auth_manager.hpp"
#include "core/error.hpp"
#include <boost/beast/core/detail/base64.hpp>
#include <format>

namespace flowdriver {

class AuthManager::Impl {
public:
    void setBasicAuth(std::string_view username, std::string_view password) {
        auth_type_ = AuthType::BASIC;
        credentials_ = std::format("{}:{}", username, password);
    }

    void setBearerToken(std::string_view token, TokenRefreshCallback refresh_callback) {
        auth_type_ = AuthType::BEARER;
        token_ = std::string(token);
        refresh_callback_ = std::move(refresh_callback);
    }

    void setApiKey(std::string_view name, std::string_view value, bool in_header) {
        auth_type_ = AuthType::API_KEY;
        api_key_name_ = std::string(name);
        api_key_value_ = std::string(value);
        api_key_in_header_ = in_header;
    }

    void applyAuth(std::vector<Header>& headers) {
        switch (auth_type_) {
            case AuthType::BASIC: {
                std::size_t encoded_size = boost::beast::detail::base64::encoded_size(credentials_.size());
                std::string encoded;
                encoded.resize(encoded_size);
                
                encoded.resize(boost::beast::detail::base64::encode(
                    encoded.data(),
                    credentials_.data(),
                    credentials_.size()
                ));
                
                headers.push_back({"Authorization", std::format("Basic {}", encoded)});
                break;
            }
            case AuthType::BEARER: {
                if (refresh_callback_ && shouldRefreshToken()) {
                    token_ = refresh_callback_();
                }
                headers.push_back({"Authorization", std::format("Bearer {}", token_)});
                break;
            }
            case AuthType::API_KEY:
                if (api_key_in_header_) {
                    headers.push_back({api_key_name_, api_key_value_});
                }
                break;
            default:
                break;
        }
    }

private:
    bool shouldRefreshToken() const {
        // Implement token expiration check
        return false;
    }

    AuthType auth_type_{AuthType::NONE};
    std::string credentials_;
    std::string token_;
    TokenRefreshCallback refresh_callback_;
    AuthConfig oauth_config_;
    std::string api_key_name_;
    std::string api_key_value_;
    bool api_key_in_header_;
};

AuthManager::AuthManager() : pimpl_(std::make_unique<Impl>()) {}
AuthManager::~AuthManager() = default;

void AuthManager::setBasicAuth(std::string_view username, std::string_view password) {
    pimpl_->setBasicAuth(username, password);
}

void AuthManager::setBearerToken(std::string_view token, TokenRefreshCallback refresh_callback) {
    pimpl_->setBearerToken(token, std::move(refresh_callback));
}

void AuthManager::setApiKey(std::string_view name, std::string_view value, bool in_header) {
    pimpl_->setApiKey(name, value, in_header);
}

void AuthManager::applyAuth(std::vector<Header>& headers) {
    pimpl_->applyAuth(headers);
}

} // namespace flowdriver
