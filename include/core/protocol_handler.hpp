#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include "core/types.hpp"
#include <future>

namespace flowdriver {

/**
 * @brief Base interface for all protocol handlers
 */
class ProtocolHandler : public QObject {
    Q_OBJECT
public:
    explicit ProtocolHandler(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~ProtocolHandler() = default;

    /**
     * @brief Asynchronously execute a request
     * @param config Request configuration
     * @return Future containing the request result
     */
    virtual std::future<RequestResult> executeAsync(const RequestConfig& config) = 0;
    
    /**
     * @brief Execute request synchronously
     * @param config Request configuration
     */
    virtual RequestResult execute(const RequestConfig& config);
    
    /**
     * @brief Cancel ongoing request if possible
     */
    virtual void cancel() = 0;

protected:
    /**
     * @brief Validate request configuration
     * @param config Configuration to validate
     * @throws Error if configuration is invalid
     */
    virtual void validateConfig(const RequestConfig& config);
};

} // namespace flowdriver 