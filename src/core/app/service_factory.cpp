#include "core/app/service_factory.h"
#include <iostream>
#include <regex>

namespace core {
namespace app {

ServiceFactory::ServiceFactory(boost::asio::any_io_executor executor)
    : executor_(executor) {
}

std::unique_ptr<Service> ServiceFactory::CreateTcpAcceptor(const ListenerConfig& config, const TcpServiceOptions& options) {
    try {
        auto tcp_server = std::make_shared<common::network::TcpAcceptor>(executor_, config.port, config.bind);
        tcp_server->SetMaxConnections(config.max_connections);
        
        // 设置连接处理器
        auto connection_handler = [options](std::shared_ptr<common::network::TcpConnector> conn) {
            if (options.on_connection) {
                options.on_connection(conn->GetConnectionId(), conn->GetRemoteEndpoint());
            }
            
            // 设置连接事件处理器
            if (options.on_message) {
                conn->SetDataHandler([options, conn_id = conn->GetConnectionId()](const std::vector<uint8_t>& data) {
                    options.on_message(conn_id, data);
                });
            }
            
            if (options.on_error) {
                conn->SetErrorHandler([options, conn_id = conn->GetConnectionId()](boost::system::error_code ec) {
                    options.on_error(conn_id, ec);
                });
            }
        };
        
        return std::make_unique<TcpAcceptorAdapter>(config.name, tcp_server);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create TCP server: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Service> ServiceFactory::CreateHttpServer(const ListenerConfig& config, const HttpServiceOptions& options) {
    try {
        auto http_config = CreateHttpServerConfig(config);
        auto http_server = std::make_unique<common::network::http::HttpServer>(executor_, http_config);
        
        // 注册请求处理器
        if (options.request_handler) {
            http_server->All(".*", [options](const common::network::http::HttpRequest& req, common::network::http::HttpResponse& resp, std::function<void()> next) {
                std::string response_body;
                // 获取URL路径
                std::string path = req.GetUrl().path;
                options.request_handler(common::network::http::HttpUtils::MethodToString(req.GetMethod()), path, req.GetHeaders(), req.GetBody(), response_body);
                resp.SetBody(response_body);
                resp.SetStatusCode(common::network::http::HttpStatusCode::OK);
                next();
            });
        }
        
        return std::make_unique<HttpServerAdapter>(config.name, std::move(http_server), false);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create HTTP server: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Service> ServiceFactory::CreateHttpsServer(const ListenerConfig& config, const HttpServiceOptions& options) {
    try {
        if (!config.ssl.has_value()) {
            std::cerr << "HTTPS server requires SSL configuration" << std::endl;
            return nullptr;
        }
        
        auto http_config = CreateHttpServerConfig(config);
        http_config.enable_ssl = true;
        http_config.ssl_certificate_file = config.ssl->cert_file;
        http_config.ssl_private_key_file = config.ssl->key_file;
        http_config.ssl_verify_client = config.ssl->verify_client;
        
        auto https_server = std::make_unique<common::network::http::HttpServer>(executor_, http_config);
        
        // 注册请求处理器
        if (options.request_handler) {
            https_server->All(".*", [options](const common::network::http::HttpRequest& req, common::network::http::HttpResponse& resp, std::function<void()> next) {
                std::string response_body;
                // 获取URL路径
                std::string path = req.GetUrl().path;
                options.request_handler(common::network::http::HttpUtils::MethodToString(req.GetMethod()), path, req.GetHeaders(), req.GetBody(), response_body);
                resp.SetBody(response_body);
                resp.SetStatusCode(common::network::http::HttpStatusCode::OK);
                next();
            });
        }
        
        return std::make_unique<HttpServerAdapter>(config.name, std::move(https_server), true);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create HTTPS server: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Service> ServiceFactory::CreateKcpAcceptor(const ListenerConfig& config, const KcpServiceOptions& options) {
    try {
        auto kcp_server = std::make_shared<common::network::KcpAcceptor>(executor_, config.port, config.bind);
        kcp_server->SetMaxConnections(config.max_connections);
        
        // 设置连接处理器
        auto connection_handler = [options](std::shared_ptr<common::network::KcpConnector> conn) {
            if (options.on_connection) {
                options.on_connection(conn->GetConnectionId(), conn->GetRemoteEndpoint());
            }
            
            // 设置连接事件处理器
            if (options.on_message) {
                conn->SetDataHandler([options, conn_id = conn->GetConnectionId()](const std::vector<uint8_t>& data) {
                    options.on_message(conn_id, data);
                });
            }
            
            if (options.on_error) {
                conn->SetErrorHandler([options, conn_id = conn->GetConnectionId()](boost::system::error_code ec) {
                    options.on_error(conn_id, ec);
                });
            }
        };
        
        return std::make_unique<KcpAcceptorAdapter>(config.name, kcp_server);
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to create KCP server: " << e.what() << std::endl;
        return nullptr;
    }
}

std::unique_ptr<Service> ServiceFactory::CreateListener(
    const ListenerConfig& config,
    const std::optional<TcpServiceOptions>& tcp_options,
    const std::optional<HttpServiceOptions>& http_options,
    const std::optional<KcpServiceOptions>& kcp_options) {
    
    if (config.type == "tcp" && tcp_options) {
        return CreateTcpAcceptor(config, *tcp_options);
    } else if (config.type == "http" && http_options) {
        return CreateHttpServer(config, *http_options);
    } else if (config.type == "https" && http_options) {
        return CreateHttpsServer(config, *http_options);
    } else if (config.type == "kcp" && kcp_options) {
        return CreateKcpAcceptor(config, *kcp_options);
    } else {
        std::cerr << "Unsupported listener type or missing options: " << config.type << std::endl;
        return nullptr;
    }
}

common::network::http::HttpServerConfig ServiceFactory::CreateHttpServerConfig(const ListenerConfig& config) {
    common::network::http::HttpServerConfig http_config;
    http_config.bind_address = config.bind;
    http_config.port = config.port;
    http_config.max_connections = config.max_connections;
    
    // 解析选项
    if (!config.options.empty()) {
        try {
            if (config.options.contains("keep_alive_timeout")) {
                auto timeout = config.options["keep_alive_timeout"].get<int>();
                http_config.keep_alive_timeout = std::chrono::seconds(timeout);
            }
            
            if (config.options.contains("request_timeout")) {
                auto timeout = config.options["request_timeout"].get<int>();
                http_config.request_timeout = std::chrono::seconds(timeout);
            }
            
            if (config.options.contains("max_request_size")) {
                http_config.max_request_size = config.options["max_request_size"].get<size_t>();
            }
            
            if (config.options.contains("enable_compression")) {
                http_config.enable_compression = config.options["enable_compression"].get<bool>();
            }
            
            if (config.options.contains("server_name")) {
                http_config.server_name = config.options["server_name"].get<std::string>();
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Error parsing HTTP server options: " << e.what() << std::endl;
        }
    }
    
    return http_config;
}

// 适配器类实现

TcpAcceptorAdapter::TcpAcceptorAdapter(const std::string& name, std::shared_ptr<common::network::TcpAcceptor> server)
    : name_(name), server_(server) {
}

bool TcpAcceptorAdapter::Start() {
    if (running_.load()) {
        return true;
    }
    
    bool success = server_->Start([](std::shared_ptr<common::network::TcpConnector> conn) {
        // 连接处理逻辑在ServiceFactory中设置
    });
    
    if (success) {
        running_.store(true);
    }
    
    return success;
}

void TcpAcceptorAdapter::Stop() {
    if (running_.load()) {
        server_->Stop();
        running_.store(false);
    }
}

bool TcpAcceptorAdapter::IsRunning() const {
    return running_.load() && server_->IsRunning();
}

const std::string& TcpAcceptorAdapter::GetName() const {
    return name_;
}

ServiceType TcpAcceptorAdapter::GetType() const {
    return ServiceType::TCP_SERVER;
}

HttpServerAdapter::HttpServerAdapter(const std::string& name, std::unique_ptr<common::network::http::HttpServer> server, bool is_https)
    : name_(name), server_(std::move(server)), is_https_(is_https) {
}

bool HttpServerAdapter::Start() {
    if (running_.load()) {
        return true;
    }
    
    bool success = server_->Start();
    if (success) {
        running_.store(true);
    }
    
    return success;
}

void HttpServerAdapter::Stop() {
    if (running_.load()) {
        server_->Stop();
        running_.store(false);
    }
}

bool HttpServerAdapter::IsRunning() const {
    return running_.load() && server_->IsRunning();
}

const std::string& HttpServerAdapter::GetName() const {
    return name_;
}

ServiceType HttpServerAdapter::GetType() const {
    return is_https_ ? ServiceType::HTTPS_SERVER : ServiceType::HTTP_SERVER;
}

KcpAcceptorAdapter::KcpAcceptorAdapter(const std::string& name, std::shared_ptr<common::network::KcpAcceptor> server)
    : name_(name), server_(server) {
}

bool KcpAcceptorAdapter::Start() {
    if (running_.load()) {
        return true;
    }
    
    bool success = server_->Start([](std::shared_ptr<common::network::KcpConnector> conn) {
        // 连接处理逻辑在ServiceFactory中设置
    });
    
    if (success) {
        running_.store(true);
    }
    
    return success;
}

void KcpAcceptorAdapter::Stop() {
    if (running_.load()) {
        server_->Stop();
        running_.store(false);
    }
}

bool KcpAcceptorAdapter::IsRunning() const {
    return running_.load() && server_->IsRunning();
}

const std::string& KcpAcceptorAdapter::GetName() const {
    return name_;
}

ServiceType KcpAcceptorAdapter::GetType() const {
    return ServiceType::KCP_SERVER;
}

// Server creation methods - aliases for existing methods
std::unique_ptr<Service> ServiceFactory::CreateTcpServer(const ListenerConfig& config, const TcpServiceOptions& options) {
    return CreateTcpAcceptor(config, options);
}

std::unique_ptr<Service> ServiceFactory::CreateKcpServer(const ListenerConfig& config, const KcpServiceOptions& options) {
    return CreateKcpAcceptor(config, options);
}

} // namespace app
} // namespace core