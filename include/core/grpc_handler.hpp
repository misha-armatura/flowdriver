#pragma once

#include "core/protocol_handler.hpp"
#include <QVariantList>
#include <grpcpp/grpcpp.h>
#include <grpcpp/generic/generic_stub.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>
#include <memory>
#include <unordered_map>
#include <future>

namespace flowdriver {

/**
 * @brief Handler for gRPC protocol communications
 */
class GrpcHandler final : public ProtocolHandler {
    Q_OBJECT
public:
    explicit GrpcHandler(QObject* parent = nullptr);
    ~GrpcHandler() override;

    // Load proto file dynamically
    void loadProtoFile(const std::string& path);
    
    // Get available services and methods
    QStringList getAvailableServices() const;
    QStringList getServiceMethods(const std::string& service) const;
    
    // Set current service and method
    void setService(const std::string& service);
    void setMethod(const std::string& method);
    
    // Configure endpoint
    void setEndpoint(const std::string& endpoint);
    void setUseSSL(bool use_ssl);
    
    // Set auth metadata
    void setAuthMetadata(const QVariantList& headers);

    // ProtocolHandler interface
    RequestResult execute(const RequestConfig& config) override;
    void cancel() override;
    std::future<RequestResult> executeAsync(const RequestConfig& config) override;

private:
    class ErrorCollector : public google::protobuf::compiler::MultiFileErrorCollector {
    public:
        void AddError(const std::string& filename, int line, int column, 
                     const std::string& message) override;
    };

    // Proto file handling
    std::unique_ptr<google::protobuf::compiler::DiskSourceTree> m_source_tree;
    std::unique_ptr<ErrorCollector> m_error_collector;
    std::unique_ptr<google::protobuf::compiler::Importer> m_importer;
    std::unique_ptr<google::protobuf::DynamicMessageFactory> m_message_factory;
    
    // Service and method info
    const google::protobuf::FileDescriptor* m_current_file{nullptr};
    const google::protobuf::ServiceDescriptor* m_current_service{nullptr};
    const google::protobuf::MethodDescriptor* m_current_method{nullptr};
    
    // Channel configuration
    std::string m_endpoint{"localhost:50051"};
    bool m_useSsl{false};
    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<grpc::GenericStub> m_generic_stub;
    
    // Service method cache
    std::unordered_map<std::string, std::vector<std::string>> m_service_methods;
    
    // Auth headers for gRPC metadata
    std::vector<Header> m_auth_headers;
    
    // Helper methods
    void createChannel();
    void executeMethod(const RequestConfig& config, RequestResult& result);

    std::string serializeRequest(const std::string& json_request);
    std::string deserializeResponse(const google::protobuf::Message* response);
};

} // namespace flowdriver 
