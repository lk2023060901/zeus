#include "redis_config_provider.h"
#include <iostream>

namespace core {
namespace app {
namespace config_providers {

std::optional<RedisConfig> RedisConfigProvider::LoadConfig(const nlohmann::json& config) {
    try {
        if (!IsConfigPresent(config)) {
            return std::nullopt;
        }
        
        RedisConfig redis_config;
        const auto& redis_json = config["services"]["redis"];
        
        if (redis_json.contains("host")) {
            redis_config.host = redis_json["host"].get<std::string>();
        }
        
        if (redis_json.contains("port")) {
            redis_config.port = redis_json["port"].get<uint16_t>();
        }
        
        if (redis_json.contains("password")) {
            redis_config.password = redis_json["password"].get<std::string>();
        }
        
        if (redis_json.contains("database")) {
            redis_config.database = redis_json["database"].get<int>();
        }
        
        if (redis_json.contains("pool_size")) {
            redis_config.pool_size = redis_json["pool_size"].get<size_t>();
        }
        
        if (redis_json.contains("timeout_ms")) {
            redis_config.timeout_ms = redis_json["timeout_ms"].get<uint32_t>();
        }
        
        if (redis_json.contains("retry_attempts")) {
            redis_config.retry_attempts = redis_json["retry_attempts"].get<int>();
        }
        
        return redis_config;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading Redis config: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool RedisConfigProvider::IsConfigPresent(const nlohmann::json& config) {
    return config.contains("services") && 
           config["services"].contains("redis") &&
           config["services"]["redis"].is_object() &&
           (!config["services"]["redis"].contains("enabled") ||
            config["services"]["redis"]["enabled"].get<bool>());
}

} // namespace config_providers
} // namespace app
} // namespace core