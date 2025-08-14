#pragma once

#include "application_types.h"
#include "app_config.h"
#include "dependency_injector.h"
#include "service_factory.h"
#include "service_registry.h"
#include "config_providers/postgresql_config_provider.h"
#include "config_providers/redis_config_provider.h"
#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>
#include <thread>

namespace core {
namespace app {

/**
 * @brief 应用程序主类，采用单例模式
 */
class Application {
public:
    /**
     * @brief 获取应用程序实例
     */
    static Application& GetInstance();
    
    /**
     * @brief 析构函数
     */
    ~Application();
    
    // 禁止拷贝和赋值
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
    /**
     * @brief 初始化应用程序
     * @param config_file 配置文件路径
     * @return 是否初始化成功
     */
    bool Initialize(const std::string& config_file = "config.json");
    
    /**
     * @brief 启动应用程序
     * @return 是否启动成功
     */
    bool Start();
    
    /**
     * @brief 运行应用程序（阻塞直到停止）
     */
    void Run();
    
    /**
     * @brief 停止应用程序
     */
    void Stop();
    
    /**
     * @brief 检查应用程序是否正在运行
     */
    bool IsRunning() const { return running_.load(); }
    
    /**
     * @brief 等待应用程序停止
     */
    void WaitForStop();
    
    // Hook注册方法
    
    /**
     * @brief 注册初始化Hook
     * @param hook Hook函数
     */
    void RegisterInitHook(hooks::InitHook hook);
    
    /**
     * @brief 注册启动Hook
     * @param hook Hook函数
     */
    void RegisterStartupHook(hooks::StartupHook hook);
    
    /**
     * @brief 注册停止Hook
     * @param hook Hook函数
     */
    void RegisterShutdownHook(hooks::ShutdownHook hook);
    
    // 信号处理方法
    
    /**
     * @brief 设置信号处理配置
     * @param config 信号处理配置
     */
    void SetSignalHandlerConfig(const SignalHandlerConfig& config);
    
    /**
     * @brief 注册信号Hook（无返回值，总是继续默认处理）
     * @param signal 信号编号
     * @param hook Hook函数
     */
    void RegisterSignalHook(int signal, hooks::SignalHook hook);
    
    /**
     * @brief 注册信号Hook（Lambda版本）
     * @param signal 信号编号
     * @param lambda Lambda函数
     */
    template<typename Lambda>
    void RegisterSignalHook(int signal, Lambda&& lambda) {
        static_assert(std::is_invocable_v<Lambda, Application&, int>, "Lambda must be callable with (Application&, int)");
        RegisterSignalHook(signal, hooks::SignalHook(std::forward<Lambda>(lambda)));
    }
    
    /**
     * @brief 注册信号处理器（可决定是否继续默认处理）
     * @param signal 信号编号
     * @param handler 处理器函数
     */
    void RegisterSignalHandler(int signal, hooks::SignalHandler handler);
    
    /**
     * @brief 注册信号处理器（Lambda版本）
     * @param signal 信号编号
     * @param lambda Lambda函数
     */
    template<typename Lambda>
    void RegisterSignalHandler(int signal, Lambda&& lambda) {
        static_assert(std::is_invocable_r_v<bool, Lambda, Application&, int>, "Lambda must be callable with (Application&, int) and return bool");
        RegisterSignalHandler(signal, hooks::SignalHandler(std::forward<Lambda>(lambda)));
    }
    
    /**
     * @brief 清除指定信号的所有处理器
     * @param signal 信号编号
     */
    void ClearSignalHandlers(int signal);
    
    /**
     * @brief 获取信号处理配置
     */
    const SignalHandlerConfig& GetSignalHandlerConfig() const { return signal_config_; }
    
    // 命令行参数解析方法
    
    /**
     * @brief 设置命令行解析配置
     * @param config 解析配置
     */
    void SetArgumentParserConfig(const ArgumentParserConfig& config);
    
    /**
     * @brief 添加命令行参数定义
     * @param definition 参数定义
     */
    void AddArgumentDefinition(const ArgumentDefinition& definition);
    
    /**
     * @brief 添加简单参数定义（仅需要参数值）
     * @param short_name 短参数名
     * @param long_name 长参数名
     * @param description 描述
     * @param requires_value 是否需要参数值
     * @param default_value 默认值
     */
    void AddArgument(const std::string& short_name, const std::string& long_name,
                    const std::string& description, bool requires_value = true,
                    const std::string& default_value = "");
    
    /**
     * @brief 添加标志参数（无需参数值）
     * @param short_name 短参数名
     * @param long_name 长参数名
     * @param description 描述
     */
    void AddFlag(const std::string& short_name, const std::string& long_name,
                const std::string& description);
    
    /**
     * @brief 添加带处理器的参数定义
     * @param short_name 短参数名
     * @param long_name 长参数名
     * @param description 描述
     * @param handler 参数处理器
     * @param requires_value 是否需要参数值
     */
    void AddArgumentWithHandler(const std::string& short_name, const std::string& long_name,
                              const std::string& description, hooks::ArgumentHandler handler,
                              bool requires_value = true);
    
    /**
     * @brief 解析命令行参数
     * @param argc 参数数量
     * @param argv 参数数组
     * @return 解析结果
     */
    ParsedArguments ParseArgs(int argc, char* argv[]);
    
    /**
     * @brief 获取解析后的参数
     */
    const ParsedArguments& GetParsedArguments() const { return parsed_args_; }
    
    /**
     * @brief 获取指定参数的值
     * @param name 参数名（短名或长名）
     * @param default_value 默认值
     * @return 参数值
     */
    std::string GetArgumentValue(const std::string& name, const std::string& default_value = "") const;
    
    /**
     * @brief 检查指定参数是否存在
     * @param name 参数名（短名或长名）
     * @return 是否存在
     */
    bool HasArgument(const std::string& name) const;
    
    /**
     * @brief 设置自定义用法显示器
     * @param provider 用法显示器
     */
    void SetUsageProvider(hooks::UsageProvider provider);
    
    /**
     * @brief 设置自定义版本显示器
     * @param provider 版本显示器
     */
    void SetVersionProvider(hooks::VersionProvider provider);
    
    /**
     * @brief 显示用法信息
     * @param program_name 程序名称
     */
    void ShowUsage(const std::string& program_name) const;
    
    /**
     * @brief 显示版本信息
     */
    void ShowVersion() const;
    
    /**
     * @brief 获取命令行配置覆盖
     * @return 解析得到的配置覆盖信息
     */
    hooks::CommandLineOverrides GetCommandLineOverrides() const;
    
    /**
     * @brief 检查是否有命令行配置覆盖
     * @return 是否存在配置覆盖
     */
    bool HasCommandLineOverrides() const;
    
    // 服务创建方法
    
    /**
     * @brief 创建TCP服务
     * @param config 监听器或连接器配置
     * @param options TCP服务选项
     * @return 是否创建成功
     */
    bool CreateTcpService(const ListenerConfig& config, const TcpServiceOptions& options);
    bool CreateTcpService(const ConnectorConfig& config, const TcpServiceOptions& options);
    
    /**
     * @brief 创建HTTP服务
     * @param config 监听器或连接器配置
     * @param options HTTP服务选项
     * @return 是否创建成功
     */
    bool CreateHttpService(const ListenerConfig& config, const HttpServiceOptions& options);
    bool CreateHttpService(const ConnectorConfig& config, const HttpServiceOptions& options);
    
    /**
     * @brief 创建KCP服务
     * @param config 监听器或连接器配置
     * @param options KCP服务选项
     * @return 是否创建成功
     */
    bool CreateKcpService(const ListenerConfig& config, const KcpServiceOptions& options);
    bool CreateKcpService(const ConnectorConfig& config, const KcpServiceOptions& options);
    
    // 访问器方法
    
    /**
     * @brief 获取应用程序配置
     */
    const AppConfig& GetConfig() const { return *config_; }
    
    /**
     * @brief 获取依赖注入容器
     */
    DependencyInjector& GetDependencyInjector() { return *di_container_; }
    
    /**
     * @brief 获取服务注册表
     */
    ServiceRegistry& GetServiceRegistry() { return *service_registry_; }
    
    /**
     * @brief 获取ASIO执行器
     */
    boost::asio::any_io_executor GetExecutor() { return io_context_.get_executor(); }
    
    /**
     * @brief 获取IO上下文
     */
    boost::asio::io_context& GetIOContext() { return io_context_; }
    
    /**
     * @brief 获取PostgreSQL配置（如果可用）
     */
    std::optional<PostgreSQLConfig> GetPostgreSQLConfig() const;
    
    /**
     * @brief 获取Redis配置（如果可用）
     */
    std::optional<RedisConfig> GetRedisConfig() const;
    
    /**
     * @brief 设置工作线程数量
     * @param thread_count 线程数量
     */
    void SetWorkerThreadCount(size_t thread_count) { worker_thread_count_ = thread_count; }
    
    /**
     * @brief 获取工作线程数量
     */
    size_t GetWorkerThreadCount() const { return worker_thread_count_; }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Application();
    
    // 初始化步骤
    bool InitializeLogging();
    bool InitializeNetworkModule();
    bool InitializeDependencyInjection();
    bool IntegrateCommandLineOverrides();
    bool InitializeServices();
    bool CallInitHooks();
    
    // 服务创建辅助方法
    bool CreateServicesFromConfig();
    bool CreateListenerServices();
    bool CreateConnectorServices();
    
    // 停止流程
    void StopServices();
    void StopWorkerThreads();
    void CallShutdownHooks();
    
    // 工作线程管理
    void StartWorkerThreads();
    void WorkerThreadFunction();
    
    // 信号处理
    void SetupSignalHandlers();
    void OnSignalReceived(const boost::system::error_code& ec, int signal);
    bool ProcessSignalHooks(int signal);
    bool ProcessSignalHandlers(int signal);
    void ExecuteDefaultSignalHandler(int signal);
    
    // 命令行参数解析私有方法
    bool ParseSingleArgument(const std::string& arg, const std::string& next_arg, 
                           int& arg_index, const std::vector<std::string>& args);
    ArgumentDefinition* FindArgumentDefinition(const std::string& name);
    std::string ResolveArgumentName(const std::string& name) const;
    void ShowDefaultUsage(const std::string& program_name) const;
    void ShowDefaultVersion() const;
    void InitializeDefaultArguments();
    
    // 命令行参数集成私有方法
    void CopyDefaultOptionsFromConfig(ListenerConfig& config) const;
    void CopyDefaultOptionsFromConfig(ConnectorConfig& config) const;
    
    // 成员变量
    std::unique_ptr<AppConfig> config_;
    std::unique_ptr<DependencyInjector> di_container_;
    std::unique_ptr<ServiceFactory> service_factory_;
    std::unique_ptr<ServiceRegistry> service_registry_;
    
    // ASIO组件
    boost::asio::io_context io_context_;
    std::unique_ptr<boost::asio::signal_set> signal_set_;
    std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> work_guard_;
    
    // 线程管理
    std::vector<std::thread> worker_threads_;
    size_t worker_thread_count_ = std::thread::hardware_concurrency();
    
    // Hook存储
    std::vector<hooks::InitHook> init_hooks_;
    std::vector<hooks::StartupHook> startup_hooks_;
    std::vector<hooks::ShutdownHook> shutdown_hooks_;
    
    // 信号处理相关
    SignalHandlerConfig signal_config_;
    std::unordered_map<int, std::vector<hooks::SignalHook>> signal_hooks_;
    std::unordered_map<int, std::vector<hooks::SignalHandler>> signal_handlers_;
    mutable std::mutex signal_mutex_;
    
    // 命令行参数解析相关
    ArgumentParserConfig arg_parser_config_;
    ParsedArguments parsed_args_;
    std::unordered_map<std::string, std::string> arg_name_mapping_; // 短名 -> 长名映射
    mutable std::mutex args_mutex_;
    
    // 状态管理
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
    
    mutable std::mutex hooks_mutex_;
    std::condition_variable stop_condition_;
    std::mutex stop_mutex_;
};

} // namespace app
} // namespace core

// 便利宏定义
#define ZEUS_APP() core::app::Application::GetInstance()
#define ZEUS_CONFIG() ZEUS_APP().GetConfig()
#define ZEUS_DI() ZEUS_APP().GetDependencyInjector()
#define ZEUS_SERVICES() ZEUS_APP().GetServiceRegistry()
#define ZEUS_EXECUTOR() ZEUS_APP().GetExecutor()