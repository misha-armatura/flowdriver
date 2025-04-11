#include "core/grpc_handler.hpp"
#include "core/error.hpp"
#include <chrono>
#include <thread>
#include <grpcpp/create_channel.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/util/json_util.h>
#include <QDebug>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace flowdriver {

using json = nlohmann::json;

void GrpcHandler::ErrorCollector::AddError(const std::string& filename, int line, int column, const std::string& message) {
    qDebug() << "Proto Error:" << QString::fromStdString(filename) << "line" << line << "column" << column << QString::fromStdString(message);
}

GrpcHandler::GrpcHandler(QObject* parent) 
    : ProtocolHandler(parent)
    , m_source_tree(std::make_unique<google::protobuf::compiler::DiskSourceTree>())
    , m_error_collector(std::make_unique<ErrorCollector>())
    , m_message_factory(std::make_unique<google::protobuf::DynamicMessageFactory>())
{
    m_importer = std::make_unique<google::protobuf::compiler::Importer>(m_source_tree.get(), m_error_collector.get());
}

GrpcHandler::~GrpcHandler() = default;

void GrpcHandler::loadProtoFile(const std::string& path) {
    try {
        std::filesystem::path proto_path(path);
        auto proto_dir = proto_path.parent_path().string();
        auto proto_file = proto_path.filename().string();
        
        // Reset and reconfigure source tree
        m_source_tree = std::make_unique<google::protobuf::compiler::DiskSourceTree>();
        m_source_tree->MapPath("", proto_dir);
        
        m_importer = std::make_unique<google::protobuf::compiler::Importer>(m_source_tree.get(), m_error_collector.get());
        
        m_current_file = m_importer->Import(proto_file);
        if (!m_current_file) {
            throw Error(ErrorCode::INVALID_ARGUMENT, "Failed to import proto file");
        }

        qDebug() << "Loaded proto file:" << QString::fromStdString(proto_file);
        qDebug() << "Package name:" << QString::fromStdString(m_current_file->package());
        qDebug() << "Number of services:" << m_current_file->service_count();
        
        for (int i = 0; i < m_current_file->service_count(); i++) {
            auto service = m_current_file->service(i);
            qDebug() << "Service" << i << ":"
                     << "name=" << QString::fromStdString(service->name())
                     << "full_name=" << QString::fromStdString(service->full_name());
        }
    } catch (const std::exception& e) {
        throw Error(ErrorCode::INVALID_ARGUMENT, "Failed to load proto file: " + std::string(e.what()));
    }
}

QStringList GrpcHandler::getAvailableServices() const {
    QStringList services;
    if (m_current_file) {
        for (int i = 0; i < m_current_file->service_count(); i++) {
            services.append(QString::fromStdString(m_current_file->service(i)->full_name()));
        }
    }
    qDebug() << "Available services:" << services;
    return services;
}

QStringList GrpcHandler::getServiceMethods(const std::string& service) const {
    QStringList methods;
    if (!m_current_file) {
        return methods;
    }
    
    qDebug() << "Looking for service methods:" << QString::fromStdString(service);
    
    // First try the service name as-is
    const google::protobuf::ServiceDescriptor* service_desc = m_current_file->FindServiceByName(service);
    
    if (!service_desc) {
        // If service contains dot, try without package name
        size_t dot_pos = service.find('.');
        if (dot_pos != std::string::npos) {
            std::string service_name = service.substr(dot_pos + 1);
            qDebug() << "Trying without package name:" << QString::fromStdString(service_name);
            service_desc = m_current_file->FindServiceByName(service_name);
        } else {
            // If no dot and no service found, try with package name
            std::string package = m_current_file->package();
            if (!package.empty()) {
                std::string full_name = package + "." + service;
                qDebug() << "Trying with package name:" << QString::fromStdString(full_name);
                service_desc = m_current_file->FindServiceByName(full_name);
            }
        }
    }
    
    if (service_desc) {
        qDebug() << "Found service descriptor with" << service_desc->method_count() << "methods";
        for (int i = 0; i < service_desc->method_count(); i++) {
            auto method = service_desc->method(i);
            qDebug() << "Adding method:" << QString::fromStdString(method->name());
            methods.append(QString::fromStdString(method->name()));
        }
    } else {
        qDebug() << "Service descriptor not found for:" << QString::fromStdString(service);
    }
    
    qDebug() << "Methods for service" << QString::fromStdString(service) << ":" << methods;
    return methods;
}

void GrpcHandler::setService(const std::string& service) {
    if (!m_current_file) {
        throw Error(ErrorCode::INVALID_ARGUMENT, "No proto file loaded");
    }
    
    qDebug() << "Looking for service:" << QString::fromStdString(service);
    
    // First try the service name as-is
    m_current_service = m_current_file->FindServiceByName(service);
    
    if (!m_current_service) {
        // If service contains dot, try without package name
        size_t dot_pos = service.find('.');
        if (dot_pos != std::string::npos) {
            std::string service_name = service.substr(dot_pos + 1);
            m_current_service = m_current_file->FindServiceByName(service_name);
        } else {
            // If no dot and no service found, try with package name
            std::string package = m_current_file->package();
            if (!package.empty()) {
                std::string full_name = package + "." + service;
                m_current_service = m_current_file->FindServiceByName(full_name);
            }
        }
    }
    
    if (!m_current_service) {
        throw Error(ErrorCode::INVALID_ARGUMENT, "Service not found: " + service);
    }
    
    qDebug() << "Found service with" << m_current_service->method_count() << "methods";
}

void GrpcHandler::setMethod(const std::string& method) {
    if (!m_current_service) {
        throw Error(ErrorCode::INVALID_STATE, "No service selected");
    }
    m_current_method = m_current_service->FindMethodByName(method);
    if (!m_current_method) {
        throw Error(ErrorCode::INVALID_CONFIG, "Method not found: " + method);
    }
}

void GrpcHandler::setEndpoint(const std::string& endpoint) {
    m_endpoint = endpoint;
    createChannel();
}

void GrpcHandler::setUseSSL(bool use_ssl) {
    m_useSsl = use_ssl;
    createChannel();
}

void GrpcHandler::createChannel() {
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, -1);
    args.SetInt(GRPC_ARG_MAX_SEND_MESSAGE_LENGTH, -1);
    
    if (m_useSsl) {
        grpc::SslCredentialsOptions ssl_opts;
        m_channel = grpc::CreateCustomChannel(m_endpoint, grpc::SslCredentials(ssl_opts), args);
    } else {
        m_channel = grpc::CreateCustomChannel(m_endpoint, grpc::InsecureChannelCredentials(), args);
    }
    
    m_generic_stub = std::make_unique<grpc::GenericStub>(m_channel);
}

RequestResult GrpcHandler::execute(const RequestConfig& config) {
    RequestResult result;
    try {
        if (!m_current_method) {
            throw Error(ErrorCode::INVALID_ARGUMENT, "No method selected");
        }
        
        if (!m_channel || !m_generic_stub) {
            createChannel();
        }
        
        executeMethod(config, result);
        return result;
    } catch (const Error& e) {
        throw;
    } catch (const std::exception& e) {
        throw Error(ErrorCode::INTERNAL_ERROR, std::string("gRPC call failed: ") + e.what());
    }
}

void GrpcHandler::executeMethod(const RequestConfig& config, RequestResult& result) {
    try {
        grpc::ClientContext context;
        std::string method_name = "/" + m_current_service->full_name() + "/" + m_current_method->name();
        
        qDebug() << "Executing gRPC method:" << QString::fromStdString(method_name);
        
        for (const auto& header : m_auth_headers) {
            context.AddMetadata(header.name, header.value);
            qDebug() << "Using auth header:" << QString::fromStdString(header.name)
                     << "=" << QString::fromStdString(header.value);
        }
        
        for (const auto& header : config.headers) {
            context.AddMetadata(header.name, header.value);
        }
        
        // Create request message
        auto request = m_message_factory->GetPrototype(m_current_method->input_type())->New();
        auto response = m_message_factory->GetPrototype(m_current_method->output_type())->New();
        
        // Parse request JSON
        std::string json_request = config.body.empty() ? "{}" : config.body; // Use empty object if no body
        auto status = google::protobuf::util::JsonStringToMessage(json_request, request);
        if (!status.ok()) {
            std::string error_msg = "Failed to parse request JSON: ";
            error_msg += status.ToString();
            qDebug() << "JSON parsing error:" << QString::fromStdString(error_msg);
            throw Error(ErrorCode::INVALID_ARGUMENT, error_msg);
        }
        
        std::string binary_request;
        if (!request->SerializeToString(&binary_request)) {
            throw Error(ErrorCode::INVALID_ARGUMENT, "Failed to serialize request");
        }
        
        // Create gRPC byte buffer from serialized request
        grpc::Slice slice(binary_request);
        grpc::ByteBuffer request_buffer(&slice, 1);
        grpc::ByteBuffer response_buffer;
        
        // Create completion queue for async operations
        grpc::CompletionQueue cq;
        std::unique_ptr<grpc::ClientAsyncResponseReader<grpc::ByteBuffer>> rpc(
            m_generic_stub->PrepareUnaryCall(&context, method_name, request_buffer, &cq));

        rpc->StartCall();
        
        grpc::Status grpc_status;
        rpc->Finish(&response_buffer, &grpc_status, (void*)1);
        
        void* got_tag;
        bool ok = false;
        cq.Next(&got_tag, &ok);
        
        if (grpc_status.ok()) {
            std::string binary_response;
            std::vector<grpc::Slice> slices;
            response_buffer.Dump(&slices);
            for (const auto& s : slices) {
                binary_response.append(reinterpret_cast<const char*>(s.begin()), s.size());
            }
            
            if (!response->ParseFromString(binary_response)) {
                throw Error(ErrorCode::INVALID_ARGUMENT, "Failed to parse response");
            }
            
            google::protobuf::util::JsonPrintOptions options;
            options.add_whitespace = true;
            options.always_print_primitive_fields = true;
            
            std::string response_json;
            status = google::protobuf::util::MessageToJsonString(*response, &response_json, options);
            if (!status.ok()) {
                throw Error(ErrorCode::INVALID_ARGUMENT, "Failed to convert response to JSON");
            }
            
            result.body = response_json;
            result.status_code = 200;
        } else {
            result.error = grpc_status.error_message();
            result.status_code = static_cast<int>(grpc_status.error_code());
        }
        
        delete request;
        delete response;
    } catch (const Error& e) {
        qDebug() << "GrpcHandler error:" << QString::fromStdString(e.what());
        throw;
    } catch (const std::exception& e) {
        qDebug() << "Unexpected error in GrpcHandler:" << e.what();
        throw Error(ErrorCode::INTERNAL_ERROR, std::string("gRPC execution failed: ") + e.what());
    }
}

void GrpcHandler::cancel() {
    // Cancel any ongoing gRPC calls by creating a new channel
    // This effectively interrupts any in-progress calls
    qDebug() << "Cancelling gRPC requests";
    
    m_channel.reset();
    m_generic_stub.reset();
    
    createChannel();
}

void GrpcHandler::setAuthMetadata(const QVariantList& headers) {
    qDebug() << "Setting gRPC auth metadata with" << headers.size() << "headers";

    m_auth_headers.clear();
    
    for (const auto& header : headers) {
        QVariantMap headerMap = header.toMap();
        if (headerMap.contains("name") && headerMap.contains("value")) {
            std::string name = headerMap["name"].toString().toStdString();
            std::string value = headerMap["value"].toString().toStdString();
            
            m_auth_headers.push_back({name, value});
            qDebug() << "Added auth header:" << QString::fromStdString(name)
                     << "=" << QString::fromStdString(value);
        }
    }
}

std::future<RequestResult> GrpcHandler::executeAsync(const RequestConfig& config) {
    return std::async(std::launch::async, [this, config]() {
        return execute(config);
    });
}

std::string GrpcHandler::serializeRequest(const std::string& json_request) {
    try {
        auto json_obj = json::parse(json_request);
        
        std::unique_ptr<google::protobuf::Message> request(
            m_message_factory->GetPrototype(m_current_method->input_type())->New()
        );
        
        // Convert JSON to protobuf
        std::string err;
        google::protobuf::util::JsonStringToMessage(json_obj.dump(), request.get());
        
        // Serialize to protobuf format
        std::string serialized;
        request->SerializeToString(&serialized);
        return serialized;
    } catch (const json::parse_error& e) {
        throw Error(ErrorCode::PARSE_ERROR, "Failed to parse JSON request: " + std::string(e.what()));
    }
}

std::string GrpcHandler::deserializeResponse(const google::protobuf::Message* response) {
    std::string json_string;
    google::protobuf::util::JsonPrintOptions options;
    options.add_whitespace = true;
    options.always_print_primitive_fields = true;
    google::protobuf::util::MessageToJsonString(*response, &json_string, options);
    
    try {
        // Parse and re-serialize to ensure proper formatting
        auto json_obj = json::parse(json_string);
        return json_obj.dump(2);
    } catch (const json::parse_error& e) {
        throw Error(ErrorCode::PARSE_ERROR, "Failed to format response JSON: " + std::string(e.what()));
    }
}

} // namespace flowdriver 
