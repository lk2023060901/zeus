#include "core/zeus_application.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace core {
namespace app {
namespace ApplicationUtils {

bool CreateDefaultConfig(const std::string& filename, const std::string& app_name) {
    try {
        nlohmann::json config;
        
        // Application section
        config["application"]["name"] = app_name;
        config["application"]["version"] = "1.0.0";
        config["application"]["lua_script_path"] = "./scripts";
        
        // Logging section
        config["logging"]["console"] = true;
        config["logging"]["file"] = true;
        config["logging"]["log_dir"] = "logs";
        
        // Example logger
        nlohmann::json logger;
        logger["name"] = "main";
        logger["level"] = "info";
        logger["console_output"] = true;
        logger["file_output"] = true;
        logger["filename_pattern"] = app_name + "_%Y%m%d.log";
        logger["rotation_type"] = "daily";
        
        config["logging"]["loggers"] = nlohmann::json::array();
        config["logging"]["loggers"].push_back(logger);
        
        // Default HTTP listener
        nlohmann::json listener;
        listener["name"] = "http_server";
        listener["type"] = "http";
        listener["port"] = 8080;
        listener["bind"] = "0.0.0.0";
        listener["max_connections"] = 1000;
        
        nlohmann::json http_options;
        http_options["keep_alive_timeout"] = 60;
        http_options["request_timeout"] = 30;
        http_options["enable_compression"] = true;
        http_options["server_name"] = "Zeus/" + std::string(ZeusApplicationVersion::VERSION_STRING);
        
        listener["options"] = http_options;
        
        config["listeners"] = nlohmann::json::array();
        config["listeners"].push_back(listener);
        
        // Services section (disabled by default)
        config["services"]["postgresql"]["enabled"] = false;
        config["services"]["postgresql"]["host"] = "localhost";
        config["services"]["postgresql"]["port"] = 5432;
        config["services"]["postgresql"]["database"] = "app_db";
        config["services"]["postgresql"]["username"] = "app_user";
        config["services"]["postgresql"]["password"] = "app_password";
        config["services"]["postgresql"]["pool_size"] = 20;
        
        config["services"]["redis"]["enabled"] = false;
        config["services"]["redis"]["host"] = "localhost";
        config["services"]["redis"]["port"] = 6379;
        config["services"]["redis"]["database"] = 0;
        config["services"]["redis"]["pool_size"] = 10;
        
        // Write to file
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Failed to create config file: " << filename << std::endl;
            return false;
        }
        
        file << config.dump(4); // Pretty print with 4-space indentation
        file.close();
        
        std::cout << "Default configuration created: " << filename << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error creating default config: " << e.what() << std::endl;
        return false;
    }
}

bool ValidateConfig(const std::string& filename) {
    try {
        if (!std::filesystem::exists(filename)) {
            std::cerr << "Configuration file not found: " << filename << std::endl;
            return false;
        }
        
        AppConfig config;
        if (!config.LoadFromFile(filename)) {
            std::cerr << "Failed to load configuration file: " << filename << std::endl;
            return false;
        }
        
        if (!config.Validate()) {
            std::cerr << "Configuration validation failed: " << filename << std::endl;
            return false;
        }
        
        std::cout << "Configuration file is valid: " << filename << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error validating config: " << e.what() << std::endl;
        return false;
    }
}

void PrintFrameworkInfo() {
    std::cout << "Zeus Application Framework" << std::endl;
    std::cout << "Version: " << ZeusApplicationVersion::VERSION_STRING << std::endl;
    std::cout << "Build Type: " << ZeusApplicationVersion::BUILD_TYPE << std::endl;
    std::cout << "Network Module: " << common::network::NetworkVersion::VERSION_STRING << std::endl;
    std::cout << std::endl;
}

ListenerConfig CreateHttpEchoServer(uint16_t port, const std::string& bind_address) {
    ListenerConfig config;
    config.name = "http_echo_server";
    config.type = "http";
    config.port = port;
    config.bind = bind_address;
    config.max_connections = 1000;
    
    // Set default HTTP options
    config.options["keep_alive_timeout"] = 60;
    config.options["request_timeout"] = 30;
    config.options["enable_compression"] = true;
    config.options["server_name"] = "Zeus-Echo/" + std::string(ZeusApplicationVersion::VERSION_STRING);
    
    return config;
}

ListenerConfig CreateTcpEchoServer(uint16_t port, const std::string& bind_address) {
    ListenerConfig config;
    config.name = "tcp_echo_server";
    config.type = "tcp";
    config.port = port;
    config.bind = bind_address;
    config.max_connections = 1000;
    
    return config;
}

} // namespace ApplicationUtils

namespace CommonHooks {

void LogApplicationInfo(Application& app) {
    const auto& app_config = app.GetConfig().GetApplicationConfig();
    std::cout << "=== Application Started ===" << std::endl;
    std::cout << "Name: " << app_config.name << std::endl;
    std::cout << "Version: " << app_config.version << std::endl;
    std::cout << "Framework: Zeus " << ZeusApplicationVersion::VERSION_STRING << std::endl;
    std::cout << "Worker Threads: " << app.GetWorkerThreadCount() << std::endl;
    std::cout << "Lua Scripts: " << app_config.lua_script_path << std::endl;
    std::cout << "===========================" << std::endl;
}

void PrintServiceStatus(Application& app) {
    const auto& registry = app.GetServiceRegistry();
    auto service_names = registry.GetServiceNames();
    
    std::cout << "=== Service Status ===" << std::endl;
    std::cout << "Total Services: " << registry.GetTotalServiceCount() << std::endl;
    std::cout << "Running Services: " << registry.GetRunningServiceCount() << std::endl;
    
    if (!service_names.empty()) {
        std::cout << "Services:" << std::endl;
        for (const auto& name : service_names) {
            auto status = registry.GetServiceStatus(name);
            if (status) {
                std::string type_name;
                switch (status->type) {
                    case ServiceType::TCP_SERVER: type_name = "TCP Server"; break;
                    case ServiceType::HTTP_SERVER: type_name = "HTTP Server"; break;
                    case ServiceType::HTTPS_SERVER: type_name = "HTTPS Server"; break;
                    case ServiceType::KCP_SERVER: type_name = "KCP Server"; break;
                    case ServiceType::TCP_CLIENT: type_name = "TCP Client"; break;
                    case ServiceType::HTTP_CLIENT: type_name = "HTTP Client"; break;
                    case ServiceType::HTTPS_CLIENT: type_name = "HTTPS Client"; break;
                    case ServiceType::KCP_CLIENT: type_name = "KCP Client"; break;
                }
                
                std::cout << "  - " << name << " (" << type_name << "): " 
                          << (status->is_running ? "RUNNING" : "STOPPED") << std::endl;
            }
        }
    }
    std::cout << "======================" << std::endl;
}

bool SetupGracefulShutdown(Application& app) {
    // The Application class already handles SIGINT and SIGTERM
    // This hook could add additional shutdown preparation
    std::cout << "Graceful shutdown handlers configured" << std::endl;
    return true;
}

bool InitializeLuaSupport(Application& app) {
    const auto& script_path = app.GetConfig().GetApplicationConfig().lua_script_path;
    
    if (!script_path.empty()) {
        // Create scripts directory if it doesn't exist
        try {
            std::filesystem::create_directories(script_path);
            std::cout << "Lua script directory ready: " << script_path << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to create Lua script directory: " << e.what() << std::endl;
            return false;
        }
    }
    
    return true;
}

void ValidateRequiredServices(Application& app) {
    const auto& registry = app.GetServiceRegistry();
    size_t running_count = registry.GetRunningServiceCount();
    size_t total_count = registry.GetTotalServiceCount();
    
    if (running_count < total_count) {
        std::cerr << "Warning: Not all services are running (" 
                  << running_count << "/" << total_count << ")" << std::endl;
    } else if (total_count == 0) {
        std::cout << "Warning: No services configured" << std::endl;
    } else {
        std::cout << "All " << total_count << " services are running successfully" << std::endl;
    }
}

void SetupDefaultHttpRoutes(Application& app) {
    // This would typically set up default HTTP routes if HTTP services exist
    // For now, just log that default routes are available
    const auto& registry = app.GetServiceRegistry();
    auto http_servers = registry.GetServicesByType(ServiceType::HTTP_SERVER);
    auto https_servers = registry.GetServicesByType(ServiceType::HTTPS_SERVER);
    
    if (!http_servers.empty() || !https_servers.empty()) {
        std::cout << "HTTP services detected. Default routes available:" << std::endl;
        std::cout << "  GET /        - Welcome message" << std::endl;
        std::cout << "  GET /health  - Health check" << std::endl;
        std::cout << "  GET /info    - Application info" << std::endl;
    }
}

} // namespace CommonHooks

namespace ConfigTemplates {

nlohmann::json WebServer(uint16_t http_port, bool enable_https, uint16_t https_port) {
    nlohmann::json config;
    
    config["application"]["name"] = "zeus_web_server";
    config["application"]["version"] = "1.0.0";
    
    config["logging"]["console"] = true;
    config["logging"]["file"] = true;
    config["logging"]["log_dir"] = "logs";
    
    // HTTP listener
    nlohmann::json http_listener;
    http_listener["name"] = "http_server";
    http_listener["type"] = "http";
    http_listener["port"] = http_port;
    http_listener["bind"] = "0.0.0.0";
    http_listener["max_connections"] = 2000;
    
    config["listeners"] = nlohmann::json::array();
    config["listeners"].push_back(http_listener);
    
    // HTTPS listener (if enabled)
    if (enable_https) {
        nlohmann::json https_listener;
        https_listener["name"] = "https_server";
        https_listener["type"] = "https";
        https_listener["port"] = https_port;
        https_listener["bind"] = "0.0.0.0";
        https_listener["max_connections"] = 2000;
        
        // SSL configuration
        https_listener["ssl"]["cert_file"] = "certs/server.crt";
        https_listener["ssl"]["key_file"] = "certs/server.key";
        https_listener["ssl"]["verify_peer"] = false;
        
        config["listeners"].push_back(https_listener);
    }
    
    return config;
}

nlohmann::json GameServer(uint16_t tcp_port, uint16_t kcp_port, uint16_t http_port) {
    nlohmann::json config;
    
    config["application"]["name"] = "zeus_game_server";
    config["application"]["version"] = "1.0.0";
    config["application"]["lua_script_path"] = "./game_scripts";
    
    config["logging"]["console"] = true;
    config["logging"]["file"] = true;
    config["logging"]["log_dir"] = "logs";
    
    config["listeners"] = nlohmann::json::array();
    
    // TCP game port
    nlohmann::json tcp_listener;
    tcp_listener["name"] = "game_tcp";
    tcp_listener["type"] = "tcp";
    tcp_listener["port"] = tcp_port;
    tcp_listener["bind"] = "0.0.0.0";
    tcp_listener["max_connections"] = 5000;
    config["listeners"].push_back(tcp_listener);
    
    // KCP game port
    nlohmann::json kcp_listener;
    kcp_listener["name"] = "game_kcp";
    kcp_listener["type"] = "kcp";
    kcp_listener["port"] = kcp_port;
    kcp_listener["bind"] = "0.0.0.0";
    kcp_listener["max_connections"] = 5000;
    config["listeners"].push_back(kcp_listener);
    
    // HTTP admin port
    nlohmann::json http_listener;
    http_listener["name"] = "admin_http";
    http_listener["type"] = "http";
    http_listener["port"] = http_port;
    http_listener["bind"] = "0.0.0.0";
    http_listener["max_connections"] = 100;
    config["listeners"].push_back(http_listener);
    
    // Database services
    config["services"]["postgresql"]["enabled"] = true;
    config["services"]["postgresql"]["host"] = "localhost";
    config["services"]["postgresql"]["database"] = "gamedb";
    config["services"]["postgresql"]["username"] = "game_user";
    
    config["services"]["redis"]["enabled"] = true;
    config["services"]["redis"]["host"] = "localhost";
    config["services"]["redis"]["database"] = 0;
    
    return config;
}

nlohmann::json Microservice(const std::string& service_name, uint16_t http_port, bool enable_database) {
    nlohmann::json config;
    
    config["application"]["name"] = service_name;
    config["application"]["version"] = "1.0.0";
    
    config["logging"]["console"] = true;
    config["logging"]["file"] = true;
    config["logging"]["log_dir"] = "logs";
    
    // HTTP API
    nlohmann::json http_listener;
    http_listener["name"] = "api_server";
    http_listener["type"] = "http";
    http_listener["port"] = http_port;
    http_listener["bind"] = "0.0.0.0";
    http_listener["max_connections"] = 1000;
    
    config["listeners"] = nlohmann::json::array();
    config["listeners"].push_back(http_listener);
    
    if (enable_database) {
        config["services"]["postgresql"]["enabled"] = true;
        config["services"]["redis"]["enabled"] = true;
    }
    
    return config;
}

nlohmann::json ApiGateway(uint16_t http_port, uint16_t https_port) {
    nlohmann::json config;
    
    config["application"]["name"] = "zeus_api_gateway";
    config["application"]["version"] = "1.0.0";
    
    config["logging"]["console"] = true;
    config["logging"]["file"] = true;
    config["logging"]["log_dir"] = "logs";
    
    config["listeners"] = nlohmann::json::array();
    
    // HTTP
    nlohmann::json http_listener;
    http_listener["name"] = "gateway_http";
    http_listener["type"] = "http";
    http_listener["port"] = http_port;
    http_listener["bind"] = "0.0.0.0";
    http_listener["max_connections"] = 5000;
    config["listeners"].push_back(http_listener);
    
    // HTTPS
    nlohmann::json https_listener;
    https_listener["name"] = "gateway_https";
    https_listener["type"] = "https";
    https_listener["port"] = https_port;
    https_listener["bind"] = "0.0.0.0";
    https_listener["max_connections"] = 5000;
    https_listener["ssl"]["cert_file"] = "certs/gateway.crt";
    https_listener["ssl"]["key_file"] = "certs/gateway.key";
    config["listeners"].push_back(https_listener);
    
    return config;
}

nlohmann::json ChatServer(uint16_t tcp_port, uint16_t http_port, uint16_t websocket_port) {
    nlohmann::json config;
    
    config["application"]["name"] = "zeus_chat_server";
    config["application"]["version"] = "1.0.0";
    
    config["logging"]["console"] = true;
    config["logging"]["file"] = true;
    config["logging"]["log_dir"] = "logs";
    
    config["listeners"] = nlohmann::json::array();
    
    // TCP chat
    nlohmann::json tcp_listener;
    tcp_listener["name"] = "chat_tcp";
    tcp_listener["type"] = "tcp";
    tcp_listener["port"] = tcp_port;
    tcp_listener["bind"] = "0.0.0.0";
    tcp_listener["max_connections"] = 10000;
    config["listeners"].push_back(tcp_listener);
    
    // HTTP API
    nlohmann::json http_listener;
    http_listener["name"] = "chat_http";
    http_listener["type"] = "http";
    http_listener["port"] = http_port;
    http_listener["bind"] = "0.0.0.0";
    http_listener["max_connections"] = 2000;
    config["listeners"].push_back(http_listener);
    
    // WebSocket (HTTP with upgrade)
    nlohmann::json ws_listener;
    ws_listener["name"] = "chat_websocket";
    ws_listener["type"] = "http";
    ws_listener["port"] = websocket_port;
    ws_listener["bind"] = "0.0.0.0";
    ws_listener["max_connections"] = 10000;
    config["listeners"].push_back(ws_listener);
    
    // Redis for chat state
    config["services"]["redis"]["enabled"] = true;
    
    return config;
}

} // namespace ConfigTemplates

} // namespace app
} // namespace core