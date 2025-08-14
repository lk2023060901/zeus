#pragma once

#include "core/app/application_types.h"

namespace core {
namespace app {
namespace config_providers {

/**
 * @brief Redis配置提供者
 */
class RedisConfigProvider : public ConfigProvider<RedisConfig> {
public:
    /**
     * @brief 从JSON配置加载Redis配置
     */
    std::optional<RedisConfig> LoadConfig(const nlohmann::json& config) override;
    
    /**
     * @brief 检查配置中是否存在Redis配置
     */
    bool IsConfigPresent(const nlohmann::json& config) override;
};

} // namespace config_providers
} // namespace app
} // namespace core