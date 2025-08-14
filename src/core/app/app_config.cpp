#include "core/app/app_config.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace core {
namespace app {

bool AppConfig::LoadFromFile(const std::string& config_file) {
    try {
        if (!std::filesystem::exists(config_file)) {
            std::cerr << "Configuration file not found: " << config_file << std::endl;
            return false;
        }
        
        std::ifstream file(config_file);
        if (!file.is_open()) {
            std::cerr << "Failed to open configuration file: " << config_file << std::endl;
            return false;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        config_file_path_ = config_file;
        return LoadFromString(content);
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file " << config_file << ": " << e.what() << std::endl;
        return false;
    }
}

bool AppConfig::LoadFromString(const std::string& json_content) {
    try {
        raw_config_ = nlohmann::json::parse(json_content);
        
        // 设置默认值
        SetDefaults();
        
        // 解析各个配置段
        if (!ParseApplicationConfig(raw_config_)) return false;
        if (!ParseLoggingConfig(raw_config_)) return false;
        if (!ParseListenerConfigs(raw_config_)) return false;
        if (!ParseConnectorConfigs(raw_config_)) return false;
        if (!ParseServiceConfigs(raw_config_)) return false;
        
        // 验证配置
        if (!Validate()) {
            std::cerr << "Configuration validation failed" << std::endl;
            return false;
        }
        
        loaded_ = true;
        return true;
        
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing configuration: " << e.what() << std::endl;
        return false;
    }
}

bool AppConfig::Validate() const {
    if (!ValidateApplicationConfig()) return false;
    if (!ValidateLoggingConfig()) return false;
    if (!ValidateNetworkConfigs()) return false;
    return true;
}

void AppConfig::SetDefaults() {
    app_config_ = ApplicationConfig{};
    logging_config_ = LoggingConfig{};
    listener_configs_.clear();
    connector_configs_.clear();
    service_configs_.clear();
}

bool AppConfig::ParseApplicationConfig(const nlohmann::json& json) {
    try {
        if (json.contains("application")) {
            const auto& app_json = json["application"];
            
            if (app_json.contains("name")) {
                app_config_.name = app_json["name"].get<std::string>();
            }
            
            if (app_json.contains("version")) {
                app_config_.version = app_json["version"].get<std::string>();
            }
            
            if (app_json.contains("lua_script_path")) {
                app_config_.lua_script_path = app_json["lua_script_path"].get<std::string>();
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing application config: " << e.what() << std::endl;
        return false;
    }
}

bool AppConfig::ParseLoggingConfig(const nlohmann::json& json) {
    try {
        if (json.contains("logging")) {
            const auto& logging_json = json["logging"];
            
            // 解析全局日志设置
            if (logging_json.contains("console")) {
                logging_config_.console = logging_json["console"].get<bool>();
            }
            
            if (logging_json.contains("file")) {
                logging_config_.file = logging_json["file"].get<bool>();
            }
            
            if (logging_json.contains("default_file_prefix")) {
                logging_config_.default_file_prefix = logging_json["default_file_prefix"].get<std::string>();
            } else if (logging_config_.file && logging_config_.default_file_prefix.empty()) {
                // 如果启用文件日志但没有指定前缀，使用应用程序名称
                logging_config_.default_file_prefix = app_config_.name;
            }
            
            if (logging_json.contains("log_dir")) {
                logging_config_.log_dir = logging_json["log_dir"].get<std::string>();
            }
            
            // 解析日志器配置
            if (logging_json.contains("loggers")) {
                const auto& loggers_json = logging_json["loggers"];
                if (loggers_json.is_array()) {
                    for (const auto& logger_json : loggers_json) {
                        LoggerConfig logger_config = ParseLoggerConfig(logger_json);
                        logging_config_.loggers.push_back(logger_config);
                    }
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing logging config: " << e.what() << std::endl;
        return false;
    }
}

LoggerConfig AppConfig::ParseLoggerConfig(const nlohmann::json& logger_json) const {
    LoggerConfig config;
    
    if (logger_json.contains("name")) {
        config.name = logger_json["name"].get<std::string>();
    }
    
    if (logger_json.contains("level")) {
        config.level = logger_json["level"].get<std::string>();
    }
    
    if (logger_json.contains("filename_pattern")) {
        config.filename_pattern = logger_json["filename_pattern"].get<std::string>();
    }
    
    if (logger_json.contains("rotation_type")) {
        config.rotation_type = logger_json["rotation_type"].get<std::string>();
    }
    
    if (logger_json.contains("console_output")) {
        config.console_output = logger_json["console_output"].get<bool>();
    }
    
    if (logger_json.contains("file_output")) {
        config.file_output = logger_json["file_output"].get<bool>();
    }
    
    if (logger_json.contains("max_file_size_mb")) {
        config.max_file_size_mb = logger_json["max_file_size_mb"].get<size_t>();
    }
    
    if (logger_json.contains("max_files")) {
        config.max_files = logger_json["max_files"].get<size_t>();
    }
    
    if (logger_json.contains("zeus_network")) {
        config.zeus_network = ParseZeusNetworkLogConfig(logger_json["zeus_network"]);
    }
    
    return config;
}

ZeusNetworkLogConfig AppConfig::ParseZeusNetworkLogConfig(const nlohmann::json& zeus_json) const {
    ZeusNetworkLogConfig config;
    
    if (zeus_json.contains("enabled")) {
        config.enabled = zeus_json["enabled"].get<bool>();
    }
    
    if (zeus_json.contains("auto_register")) {
        config.auto_register = zeus_json["auto_register"].get<bool>();
    }
    
    if (zeus_json.contains("event_logging")) {
        const auto& event_json = zeus_json["event_logging"];
        
        if (event_json.contains("connection_events")) {
            config.event_logging.connection_events = event_json["connection_events"].get<bool>();
        }
        
        if (event_json.contains("data_transfer")) {
            config.event_logging.data_transfer = event_json["data_transfer"].get<bool>();
        }
        
        if (event_json.contains("error_events")) {
            config.event_logging.error_events = event_json["error_events"].get<bool>();
        }
        
        if (event_json.contains("performance_metrics")) {
            config.event_logging.performance_metrics = event_json["performance_metrics"].get<bool>();
        }
    }
    
    if (zeus_json.contains("filters")) {
        const auto& filters_json = zeus_json["filters"];
        
        if (filters_json.contains("min_data_size")) {
            config.filters.min_data_size = filters_json["min_data_size"].get<size_t>();
        }
        
        if (filters_json.contains("excluded_events")) {
            config.filters.excluded_events = filters_json["excluded_events"].get<std::vector<std::string>>();
        }
        
        if (filters_json.contains("include_connection_types")) {
            config.filters.include_connection_types = filters_json["include_connection_types"].get<std::vector<std::string>>();
        }
    }
    
    return config;
}

bool AppConfig::ParseListenerConfigs(const nlohmann::json& json) {
    try {
        if (json.contains("listeners")) {
            const auto& listeners_json = json["listeners"];
            if (listeners_json.is_array()) {
                for (const auto& listener_json : listeners_json) {
                    ListenerConfig config;
                    
                    if (listener_json.contains("name")) {
                        config.name = listener_json["name"].get<std::string>();
                    }
                    
                    if (listener_json.contains("type")) {
                        config.type = listener_json["type"].get<std::string>();
                    }
                    
                    if (listener_json.contains("port")) {
                        config.port = listener_json["port"].get<uint16_t>();
                    }
                    
                    if (listener_json.contains("bind")) {
                        config.bind = listener_json["bind"].get<std::string>();
                    }
                    
                    if (listener_json.contains("max_connections")) {
                        config.max_connections = listener_json["max_connections"].get<size_t>();
                    }
                    
                    if (listener_json.contains("ssl")) {
                        config.ssl = ParseSSLConfig(listener_json["ssl"]);
                    }
                    
                    if (listener_json.contains("options")) {
                        config.options = listener_json["options"];
                    }
                    
                    listener_configs_.push_back(config);
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing listener configs: " << e.what() << std::endl;
        return false;
    }
}

bool AppConfig::ParseConnectorConfigs(const nlohmann::json& json) {
    try {
        if (json.contains("connectors")) {
            const auto& connectors_json = json["connectors"];
            if (connectors_json.is_array()) {
                for (const auto& connector_json : connectors_json) {
                    ConnectorConfig config;
                    
                    if (connector_json.contains("name")) {
                        config.name = connector_json["name"].get<std::string>();
                    }
                    
                    if (connector_json.contains("type")) {
                        config.type = connector_json["type"].get<std::string>();
                    }
                    
                    if (connector_json.contains("targets")) {
                        config.targets = connector_json["targets"].get<std::vector<std::string>>();
                    }
                    
                    if (connector_json.contains("ssl")) {
                        config.ssl = ParseSSLConfig(connector_json["ssl"]);
                    }
                    
                    if (connector_json.contains("options")) {
                        config.options = connector_json["options"];
                    }
                    
                    connector_configs_.push_back(config);
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing connector configs: " << e.what() << std::endl;
        return false;
    }
}

bool AppConfig::ParseServiceConfigs(const nlohmann::json& json) {
    try {
        if (json.contains("services")) {
            const auto& services_json = json["services"];
            if (services_json.is_object()) {
                for (auto it = services_json.begin(); it != services_json.end(); ++it) {
                    ServiceConfig config;
                    const auto& service_json = it.value();
                    
                    if (service_json.contains("enabled")) {
                        config.enabled = service_json["enabled"].get<bool>();
                    }
                    
                    if (service_json.contains("config_provider")) {
                        config.config_provider = service_json["config_provider"].get<std::string>();
                    }
                    
                    if (service_json.contains("options")) {
                        config.options = service_json["options"];
                    } else {
                        config.options = service_json; // 整个配置就是选项
                    }
                    
                    service_configs_[it.key()] = config;
                }
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing service configs: " << e.what() << std::endl;
        return false;
    }
}

SSLConfig AppConfig::ParseSSLConfig(const nlohmann::json& ssl_json) const {
    SSLConfig config;
    
    if (ssl_json.contains("cert_file")) {
        config.cert_file = ssl_json["cert_file"].get<std::string>();
    }
    
    if (ssl_json.contains("key_file")) {
        config.key_file = ssl_json["key_file"].get<std::string>();
    }
    
    if (ssl_json.contains("ca_file")) {
        config.ca_file = ssl_json["ca_file"].get<std::string>();
    }
    
    if (ssl_json.contains("verify_peer")) {
        config.verify_peer = ssl_json["verify_peer"].get<bool>();
    }
    
    if (ssl_json.contains("verify_client")) {
        config.verify_client = ssl_json["verify_client"].get<bool>();
    }
    
    return config;
}

bool AppConfig::ValidateApplicationConfig() const {
    if (app_config_.name.empty()) {
        std::cerr << "Application name cannot be empty" << std::endl;
        return false;
    }
    
    if (app_config_.version.empty()) {
        std::cerr << "Application version cannot be empty" << std::endl;
        return false;
    }
    
    return true;
}

bool AppConfig::ValidateLoggingConfig() const {
    // 如果启用文件日志，确保有有效的前缀
    if (logging_config_.file && logging_config_.default_file_prefix.empty()) {
        std::cerr << "File logging enabled but no default file prefix provided" << std::endl;
        return false;
    }
    
    // 验证每个日志器配置
    for (const auto& logger : logging_config_.loggers) {
        if (logger.name.empty()) {
            std::cerr << "Logger name cannot be empty" << std::endl;
            return false;
        }
    }
    
    return true;
}

bool AppConfig::ValidateNetworkConfigs() const {
    // 至少需要一个监听器或连接器配置
    if (listener_configs_.empty() && connector_configs_.empty()) {
        std::cout << "Warning: No listeners or connectors configured. Will use default port settings." << std::endl;
    }
    
    // 验证监听器配置
    for (const auto& listener : listener_configs_) {
        if (!ValidateListenerConfig(listener)) {
            return false;
        }
    }
    
    // 验证连接器配置
    for (const auto& connector : connector_configs_) {
        if (!ValidateConnectorConfig(connector)) {
            return false;
        }
    }
    
    return true;
}

bool AppConfig::ValidateListenerConfig(const ListenerConfig& config) const {
    if (config.name.empty()) {
        std::cerr << "Listener name cannot be empty" << std::endl;
        return false;
    }
    
    if (config.type.empty()) {
        std::cerr << "Listener type cannot be empty" << std::endl;
        return false;
    }
    
    // 验证支持的类型
    std::vector<std::string> valid_types = {"tcp", "http", "https", "kcp"};
    if (std::find(valid_types.begin(), valid_types.end(), config.type) == valid_types.end()) {
        std::cerr << "Invalid listener type: " << config.type << std::endl;
        return false;
    }
    
    if (config.port == 0) {
        std::cerr << "Invalid port number: " << config.port << std::endl;
        return false;
    }
    
    return true;
}

bool AppConfig::ValidateConnectorConfig(const ConnectorConfig& config) const {
    if (config.name.empty()) {
        std::cerr << "Connector name cannot be empty" << std::endl;
        return false;
    }
    
    if (config.type.empty()) {
        std::cerr << "Connector type cannot be empty" << std::endl;
        return false;
    }
    
    if (config.targets.empty()) {
        std::cerr << "Connector must have at least one target" << std::endl;
        return false;
    }
    
    return true;
}

std::optional<PostgreSQLConfig> AppConfig::GetPostgreSQLConfig() const {
    auto it = service_configs_.find("postgresql");
    if (it != service_configs_.end() && it->second.enabled) {
        try {
            PostgreSQLConfig config;
            const auto& options = it->second.options;
            
            if (options.contains("host")) {
                config.host = options["host"].get<std::string>();
            }
            
            if (options.contains("port")) {
                config.port = options["port"].get<uint16_t>();
            }
            
            if (options.contains("database")) {
                config.database = options["database"].get<std::string>();
            }
            
            if (options.contains("username")) {
                config.username = options["username"].get<std::string>();
            }
            
            if (options.contains("password")) {
                config.password = options["password"].get<std::string>();
            }
            
            if (options.contains("pool_size")) {
                config.pool_size = options["pool_size"].get<size_t>();
            }
            
            if (options.contains("timeout_seconds")) {
                config.timeout_seconds = options["timeout_seconds"].get<uint32_t>();
            }
            
            if (options.contains("ssl_mode")) {
                config.ssl_mode = options["ssl_mode"].get<std::string>();
            }
            
            return config;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing PostgreSQL config: " << e.what() << std::endl;
        }
    }
    
    return std::nullopt;
}

std::optional<RedisConfig> AppConfig::GetRedisConfig() const {
    auto it = service_configs_.find("redis");
    if (it != service_configs_.end() && it->second.enabled) {
        try {
            RedisConfig config;
            const auto& options = it->second.options;
            
            if (options.contains("host")) {
                config.host = options["host"].get<std::string>();
            }
            
            if (options.contains("port")) {
                config.port = options["port"].get<uint16_t>();
            }
            
            if (options.contains("password")) {
                config.password = options["password"].get<std::string>();
            }
            
            if (options.contains("database")) {
                config.database = options["database"].get<int>();
            }
            
            if (options.contains("pool_size")) {
                config.pool_size = options["pool_size"].get<size_t>();
            }
            
            if (options.contains("timeout_ms")) {
                config.timeout_ms = options["timeout_ms"].get<uint32_t>();
            }
            
            if (options.contains("retry_attempts")) {
                config.retry_attempts = options["retry_attempts"].get<int>();
            }
            
            return config;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing Redis config: " << e.what() << std::endl;
        }
    }
    
    return std::nullopt;
}

bool AppConfig::HasConfigSection(const std::string& path) const {
    try {
        return raw_config_.contains(nlohmann::json::json_pointer("/" + path));
    } catch (const std::exception&) {
        return false;
    }
}

std::optional<nlohmann::json> AppConfig::GetConfigSection(const std::string& path) const {
    try {
        auto pointer = nlohmann::json::json_pointer("/" + path);
        if (raw_config_.contains(pointer)) {
            return raw_config_[pointer];
        }
    } catch (const std::exception&) {
        // 路径无效
    }
    
    return std::nullopt;
}

nlohmann::json AppConfig::GenerateDefaultConfig() {
    nlohmann::json config;
    
    // Application配置
    config["application"]["name"] = "app";
    config["application"]["version"] = "1.0.0";
    config["application"]["lua_script_path"] = "./scripts";
    
    // Logging配置
    config["logging"]["console"] = true;
    config["logging"]["file"] = true;
    config["logging"]["log_dir"] = "logs";
    config["logging"]["loggers"] = nlohmann::json::array();
    
    // 示例listener配置
    nlohmann::json listener;
    listener["name"] = "http_server";
    listener["type"] = "http";
    listener["port"] = 8080;
    listener["bind"] = "0.0.0.0";
    listener["max_connections"] = 1000;
    
    config["listeners"] = nlohmann::json::array();
    config["listeners"].push_back(listener);
    
    // 示例服务配置
    config["services"]["postgresql"]["enabled"] = false;
    config["services"]["redis"]["enabled"] = false;
    
    return config;
}

void AppConfig::AddListenerConfig(const ListenerConfig& config) {
    listener_configs_.push_back(config);
}

void AppConfig::AddConnectorConfig(const ConnectorConfig& config) {
    connector_configs_.push_back(config);
}

bool AppConfig::HasPortConflict(uint16_t port, const std::string& bind_address) const {
    for (const auto& listener : listener_configs_) {
        if (listener.port == port && 
            (listener.bind == bind_address || listener.bind == "0.0.0.0" || bind_address == "0.0.0.0")) {
            return true;
        }
    }
    return false;
}

} // namespace app
} // namespace core