#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <optional>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>

namespace core {
namespace app {

// Forward declarations
class Application;

/**
 * @brief 应用程序配置结构
 */
struct ApplicationConfig {
    std::string name = "app";
    std::string version = "1.0.0";
    std::string lua_script_path = "./scripts";
};

/**
 * @brief SSL配置结构
 */
struct SSLConfig {
    std::string cert_file;
    std::string key_file;
    std::string ca_file;
    bool verify_peer = false;
    bool verify_client = false;
};

/**
 * @brief 监听器配置结构
 */
struct ListenerConfig {
    std::string name;
    std::string type;           // tcp, http, https, kcp
    uint16_t port = 0;
    std::string bind = "0.0.0.0";
    std::optional<SSLConfig> ssl;
    nlohmann::json options;     // 类型特定选项
    size_t max_connections = 1000;
};

/**
 * @brief 连接器配置结构
 */
struct ConnectorConfig {
    std::string name;
    std::string type;           // tcp, http, https, kcp
    std::vector<std::string> targets;
    std::optional<SSLConfig> ssl;
    nlohmann::json options;     // 类型特定选项
};

/**
 * @brief Zeus网络日志配置
 */
struct ZeusNetworkLogConfig {
    bool enabled = false;
    bool auto_register = true;
    
    struct EventLogging {
        bool connection_events = true;
        bool data_transfer = true;
        bool error_events = true;
        bool performance_metrics = false;
    } event_logging;
    
    struct Filters {
        size_t min_data_size = 0;
        std::vector<std::string> excluded_events;
        std::vector<std::string> include_connection_types;
    } filters;
};

/**
 * @brief 日志器配置结构
 */
struct LoggerConfig {
    std::string name;
    std::string level = "info";
    std::string filename_pattern;
    std::string rotation_type = "daily";
    bool console_output = true;
    bool file_output = true;
    size_t max_file_size_mb = 100;
    size_t max_files = 10;
    
    // Zeus网络专用配置
    std::optional<ZeusNetworkLogConfig> zeus_network;
};

/**
 * @brief 日志配置结构
 */
struct LoggingConfig {
    bool console = true;
    bool file = true;
    std::string default_file_prefix;
    std::string log_dir = "logs";
    std::vector<LoggerConfig> loggers;
};

/**
 * @brief 服务配置结构
 */
struct ServiceConfig {
    bool enabled = true;
    std::string config_provider;
    nlohmann::json options;
};

/**
 * @brief PostgreSQL配置结构
 */
struct PostgreSQLConfig {
    std::string host = "localhost";
    uint16_t port = 5432;
    std::string database;
    std::string username;
    std::string password;
    size_t pool_size = 20;
    uint32_t timeout_seconds = 30;
    std::string ssl_mode = "prefer";
};

/**
 * @brief Redis配置结构
 */
struct RedisConfig {
    std::string host = "localhost";
    uint16_t port = 6379;
    std::string password;
    int database = 0;
    size_t pool_size = 10;
    uint32_t timeout_ms = 5000;
    int retry_attempts = 3;
};

// Hook和回调函数类型定义
namespace hooks {

/**
 * @brief 应用程序生命周期Hooks
 */
using InitHook = std::function<bool(Application&)>;
using StartupHook = std::function<void(Application&)>;
using ShutdownHook = std::function<void(Application&)>;

/**
 * @brief 信号处理Hooks
 */
using SignalHook = std::function<void(Application&, int signal)>;
using SignalHandler = std::function<bool(Application&, int signal)>; // 返回true表示继续默认处理，false表示已处理

/**
 * @brief 命令行参数解析Hooks
 */
using ArgumentHandler = std::function<bool(Application&, const std::string& arg, const std::string& value)>;
using UsageProvider = std::function<void(const std::string& program_name)>;
using VersionProvider = std::function<void()>;

/**
 * @brief 监听端点配置
 */
struct ListenEndpoint {
    std::string protocol;  // tcp, kcp, http, https
    std::string address;
    uint16_t port;
    
    std::string ToString() const {
        return protocol + "://" + address + ":" + std::to_string(port);
    }
    
    static std::optional<ListenEndpoint> Parse(const std::string& str);
};

/**
 * @brief 命令行配置覆盖结果
 */
struct CommandLineOverrides {
    std::vector<ListenEndpoint> listen_endpoints;
    std::vector<std::string> backend_servers;
    std::optional<std::string> log_level;
    std::optional<size_t> max_connections;
    std::optional<uint32_t> timeout_ms;
    bool daemon_mode = false;
    
    bool HasOverrides() const {
        return !listen_endpoints.empty() || 
               !backend_servers.empty() || 
               log_level.has_value() || 
               max_connections.has_value() || 
               timeout_ms.has_value() || 
               daemon_mode;
    }
};

/**
 * @brief 网络事件Hooks
 */
using ConnectionHook = std::function<void(const std::string& connection_id, const std::string& endpoint)>;
using DisconnectionHook = std::function<void(const std::string& connection_id, boost::system::error_code ec)>;
using MessageHook = std::function<void(const std::string& connection_id, const std::vector<uint8_t>& data)>;
using ErrorHook = std::function<void(const std::string& connection_id, boost::system::error_code ec)>;

/**
 * @brief 消息解析器
 */
using MessageParser = std::function<std::vector<uint8_t>(const std::vector<uint8_t>& raw_data)>;

/**
 * @brief HTTP请求处理器
 */
using HttpRequestHandler = std::function<void(const std::string& method, const std::string& path, 
                                             const std::unordered_map<std::string, std::string>& headers,
                                             const std::string& body, std::string& response)>;

} // namespace hooks

/**
 * @brief TCP服务选项
 */
struct TcpServiceOptions {
    hooks::ConnectionHook on_connection;
    hooks::DisconnectionHook on_disconnect;
    hooks::MessageHook on_message;
    hooks::ErrorHook on_error;
    hooks::MessageParser message_parser;
};

/**
 * @brief HTTP服务选项
 */
struct HttpServiceOptions {
    hooks::HttpRequestHandler request_handler;
    std::function<bool(const std::string&, const std::unordered_map<std::string, std::string>&)> auth_handler;
    hooks::ErrorHook error_handler;
};

/**
 * @brief KCP服务选项
 */
struct KcpServiceOptions {
    hooks::ConnectionHook on_connection;
    hooks::DisconnectionHook on_disconnect;
    hooks::MessageHook on_message;
    hooks::ErrorHook on_error;
    hooks::MessageParser message_parser;
    uint32_t conv_id_start = 1000;
    uint32_t conv_id_end = 9999;
};

/**
 * @brief 服务类型枚举
 */
enum class ServiceType {
    TCP_SERVER,
    HTTP_SERVER,
    HTTPS_SERVER,
    KCP_SERVER,
    TCP_CLIENT,
    HTTP_CLIENT,
    HTTPS_CLIENT,
    KCP_CLIENT
};

/**
 * @brief 信号处理策略枚举
 */
enum class SignalHandlerStrategy {
    DEFAULT_ONLY,      // 仅使用默认处理
    HOOK_FIRST,       // Hook优先，然后默认处理
    HOOK_ONLY,        // 仅使用Hook处理
    HOOK_OVERRIDE     // Hook决定是否继续默认处理
};

/**
 * @brief 信号处理配置结构
 */
struct SignalHandlerConfig {
    SignalHandlerStrategy strategy = SignalHandlerStrategy::DEFAULT_ONLY;
    std::vector<int> handled_signals = {SIGINT, SIGTERM}; // 默认处理的信号
    bool graceful_shutdown = true;     // 是否优雅关闭
    uint32_t shutdown_timeout_ms = 30000; // 关闭超时时间
    bool log_signal_events = true;     // 是否记录信号事件
};

/**
 * @brief 命令行参数定义结构
 */
struct ArgumentDefinition {
    std::string short_name;    // 短参数名 (e.g., "c")
    std::string long_name;     // 长参数名 (e.g., "config")
    std::string description;   // 参数描述
    bool requires_value = false; // 是否需要参数值
    bool is_flag = false;      // 是否为标志参数
    std::string default_value; // 默认值
    hooks::ArgumentHandler handler;   // 参数处理器
};

/**
 * @brief 命令行解析结果
 */
struct ParsedArguments {
    std::unordered_map<std::string, std::string> values; // 参数名 -> 参数值
    std::vector<std::string> positional_args;           // 位置参数
    bool help_requested = false;                         // 是否请求帮助
    bool version_requested = false;                      // 是否请求版本信息
    std::string error_message;                           // 解析错误信息
};

/**
 * @brief 命令行解析配置
 */
struct ArgumentParserConfig {
    std::string program_name;                    // 程序名称
    std::string program_description;             // 程序描述
    std::string program_version = "1.0.0";        // 程序版本
    std::vector<ArgumentDefinition> arguments;   // 参数定义列表
    hooks::UsageProvider usage_provider;               // 自定义用法显示器
    hooks::VersionProvider version_provider;           // 自定义版本显示器
    bool auto_add_help = true;                  // 自动添加--help参数
    bool auto_add_version = true;               // 自动添加--version参数
};

/**
 * @brief 配置提供者接口
 */
template<typename ConfigType>
class ConfigProvider {
public:
    virtual ~ConfigProvider() = default;
    virtual std::optional<ConfigType> LoadConfig(const nlohmann::json& config) = 0;
    virtual bool IsConfigPresent(const nlohmann::json& config) = 0;
};

/**
 * @brief 服务接口
 */
class Service {
public:
    virtual ~Service() = default;
    virtual bool Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() const = 0;
    virtual const std::string& GetName() const = 0;
    virtual ServiceType GetType() const = 0;
};

} // namespace app
} // namespace core