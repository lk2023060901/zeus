#pragma once

#include "application_types.h"
#include <string>
#include <optional>
#include <vector>
#include <memory>

namespace core {
namespace app {

/**
 * @brief 应用程序配置管理类
 */
class AppConfig {
public:
    /**
     * @brief 构造函数
     */
    AppConfig() = default;
    
    /**
     * @brief 析构函数
     */
    ~AppConfig() = default;
    
    // 禁止拷贝和赋值
    AppConfig(const AppConfig&) = delete;
    AppConfig& operator=(const AppConfig&) = delete;
    
    /**
     * @brief 从JSON文件加载配置
     * @param config_file 配置文件路径
     * @return 是否加载成功
     */
    bool LoadFromFile(const std::string& config_file);
    
    /**
     * @brief 从JSON字符串加载配置
     * @param json_content JSON字符串内容
     * @return 是否加载成功
     */
    bool LoadFromString(const std::string& json_content);
    
    /**
     * @brief 验证配置是否有效
     * @return 是否有效
     */
    bool Validate() const;
    
    /**
     * @brief 获取应用程序配置
     */
    const ApplicationConfig& GetApplicationConfig() const { return app_config_; }
    
    /**
     * @brief 获取日志配置
     */
    const LoggingConfig& GetLoggingConfig() const { return logging_config_; }
    
    /**
     * @brief 获取监听器配置列表
     */
    const std::vector<ListenerConfig>& GetListenerConfigs() const { return listener_configs_; }
    
    /**
     * @brief 获取连接器配置列表
     */
    const std::vector<ConnectorConfig>& GetConnectorConfigs() const { return connector_configs_; }
    
    /**
     * @brief 获取服务配置映射
     */
    const std::unordered_map<std::string, ServiceConfig>& GetServiceConfigs() const { return service_configs_; }
    
    /**
     * @brief 获取PostgreSQL配置（如果存在）
     */
    std::optional<PostgreSQLConfig> GetPostgreSQLConfig() const;
    
    /**
     * @brief 获取Redis配置（如果存在）
     */
    std::optional<RedisConfig> GetRedisConfig() const;
    
    /**
     * @brief 获取原始JSON配置
     */
    const nlohmann::json& GetRawConfig() const { return raw_config_; }
    
    /**
     * @brief 添加监听器配置
     * @param config 监听器配置
     */
    void AddListenerConfig(const ListenerConfig& config);
    
    /**
     * @brief 添加连接器配置
     * @param config 连接器配置
     */
    void AddConnectorConfig(const ConnectorConfig& config);
    
    /**
     * @brief 检查端口冲突
     * @param port 端口号
     * @param bind_address 绑定地址
     * @return 是否存在冲突
     */
    bool HasPortConflict(uint16_t port, const std::string& bind_address) const;
    
    /**
     * @brief 检查配置项是否存在
     * @param path JSON路径（如 "database.postgresql"）
     */
    bool HasConfigSection(const std::string& path) const;
    
    /**
     * @brief 获取配置项的JSON值
     * @param path JSON路径
     */
    std::optional<nlohmann::json> GetConfigSection(const std::string& path) const;
    
    /**
     * @brief 获取配置文件路径
     */
    const std::string& GetConfigFilePath() const { return config_file_path_; }
    
    /**
     * @brief 设置默认值
     */
    void SetDefaults();
    
    /**
     * @brief 生成默认配置JSON
     */
    static nlohmann::json GenerateDefaultConfig();

private:
    // 解析方法
    bool ParseApplicationConfig(const nlohmann::json& json);
    bool ParseLoggingConfig(const nlohmann::json& json);
    bool ParseListenerConfigs(const nlohmann::json& json);
    bool ParseConnectorConfigs(const nlohmann::json& json);
    bool ParseServiceConfigs(const nlohmann::json& json);
    
    // 验证方法
    bool ValidateApplicationConfig() const;
    bool ValidateLoggingConfig() const;
    bool ValidateNetworkConfigs() const;
    bool ValidateListenerConfig(const ListenerConfig& config) const;
    bool ValidateConnectorConfig(const ConnectorConfig& config) const;
    
    // 工具方法
    SSLConfig ParseSSLConfig(const nlohmann::json& ssl_json) const;
    LoggerConfig ParseLoggerConfig(const nlohmann::json& logger_json) const;
    ZeusNetworkLogConfig ParseZeusNetworkLogConfig(const nlohmann::json& zeus_json) const;
    std::string GetDefaultLogFileName() const;
    
    // 配置数据
    ApplicationConfig app_config_;
    LoggingConfig logging_config_;
    std::vector<ListenerConfig> listener_configs_;
    std::vector<ConnectorConfig> connector_configs_;
    std::unordered_map<std::string, ServiceConfig> service_configs_;
    
    // 原始数据
    nlohmann::json raw_config_;
    std::string config_file_path_;
    bool loaded_ = false;
};

} // namespace app
} // namespace core