#pragma once

#include "core/app/application_types.h"

namespace core {
namespace app {
namespace config_providers {

/**
 * @brief PostgreSQL配置提供者
 */
class PostgreSQLConfigProvider : public ConfigProvider<PostgreSQLConfig> {
public:
    /**
     * @brief 从JSON配置加载PostgreSQL配置
     */
    std::optional<PostgreSQLConfig> LoadConfig(const nlohmann::json& config) override;
    
    /**
     * @brief 检查配置中是否存在PostgreSQL配置
     */
    bool IsConfigPresent(const nlohmann::json& config) override;
};

} // namespace config_providers
} // namespace app
} // namespace core