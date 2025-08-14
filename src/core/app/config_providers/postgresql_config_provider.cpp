#include "postgresql_config_provider.h"
#include <iostream>

namespace core {
namespace app {
namespace config_providers {

std::optional<PostgreSQLConfig> PostgreSQLConfigProvider::LoadConfig(const nlohmann::json& config) {
    try {
        if (!IsConfigPresent(config)) {
            return std::nullopt;
        }
        
        PostgreSQLConfig pg_config;
        const auto& pg_json = config["services"]["postgresql"];
        
        if (pg_json.contains("host")) {
            pg_config.host = pg_json["host"].get<std::string>();
        }
        
        if (pg_json.contains("port")) {
            pg_config.port = pg_json["port"].get<uint16_t>();
        }
        
        if (pg_json.contains("database")) {
            pg_config.database = pg_json["database"].get<std::string>();
        }
        
        if (pg_json.contains("username")) {
            pg_config.username = pg_json["username"].get<std::string>();
        }
        
        if (pg_json.contains("password")) {
            pg_config.password = pg_json["password"].get<std::string>();
        }
        
        if (pg_json.contains("pool_size")) {
            pg_config.pool_size = pg_json["pool_size"].get<size_t>();
        }
        
        if (pg_json.contains("timeout_seconds")) {
            pg_config.timeout_seconds = pg_json["timeout_seconds"].get<uint32_t>();
        }
        
        if (pg_json.contains("ssl_mode")) {
            pg_config.ssl_mode = pg_json["ssl_mode"].get<std::string>();
        }
        
        // 验证必需字段
        if (pg_config.database.empty()) {
            std::cerr << "PostgreSQL database name is required" << std::endl;
            return std::nullopt;
        }
        
        if (pg_config.username.empty()) {
            std::cerr << "PostgreSQL username is required" << std::endl;
            return std::nullopt;
        }
        
        return pg_config;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading PostgreSQL config: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool PostgreSQLConfigProvider::IsConfigPresent(const nlohmann::json& config) {
    return config.contains("services") && 
           config["services"].contains("postgresql") &&
           config["services"]["postgresql"].is_object() &&
           (!config["services"]["postgresql"].contains("enabled") ||
            config["services"]["postgresql"]["enabled"].get<bool>());
}

} // namespace config_providers
} // namespace app
} // namespace core