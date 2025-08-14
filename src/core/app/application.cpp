#include "core/app/application.h"
#include "common/spdlog/zeus_log_config.h"
#include "common/network/zeus_network.h"
#include <iostream>
#include <iomanip>
#include <csignal>

namespace core {
namespace app {

Application& Application::GetInstance() {
    static Application instance;
    return instance;
}

Application::Application()
    : config_(std::make_unique<AppConfig>())
    , di_container_(std::make_unique<DependencyInjector>())
    , service_factory_(nullptr)
    , service_registry_(std::make_unique<ServiceRegistry>()) {
}

Application::~Application() {
    if (running_.load()) {
        Stop();
    }
}

bool Application::Initialize(const std::string& config_file) {
    if (initialized_.load()) {
        std::cout << "Application already initialized" << std::endl;
        return true;
    }
    
    std::cout << "Initializing application..." << std::endl;
    
    // 1. 加载配置文件
    if (!config_->LoadFromFile(config_file)) {
        std::cerr << "Failed to load configuration from " << config_file << std::endl;
        return false;
    }
    
    std::cout << "Configuration loaded: " << config_->GetApplicationConfig().name 
              << " v" << config_->GetApplicationConfig().version << std::endl;
    
    // 2. 初始化日志系统
    if (!InitializeLogging()) {
        std::cerr << "Failed to initialize logging" << std::endl;
        return false;
    }
    
    // 3. 初始化网络模块
    if (!InitializeNetworkModule()) {
        std::cerr << "Failed to initialize network module" << std::endl;
        return false;
    }
    
    // 4. 创建服务工厂
    service_factory_ = std::make_unique<ServiceFactory>(GetExecutor());
    
    // 5. 初始化依赖注入
    if (!InitializeDependencyInjection()) {
        std::cerr << "Failed to initialize dependency injection" << std::endl;
        return false;
    }
    
    // 5.5. 将命令行参数集成到配置系统中
    if (!IntegrateCommandLineOverrides()) {
        std::cerr << "Failed to integrate command line overrides" << std::endl;
        return false;
    }
    
    // 6. 初始化服务
    if (!InitializeServices()) {
        std::cerr << "Failed to initialize services" << std::endl;
        return false;
    }
    
    // 7. 调用初始化Hooks
    if (!CallInitHooks()) {
        std::cerr << "Init hooks failed" << std::endl;
        return false;
    }
    
    // 8. 设置信号处理
    SetupSignalHandlers();
    
    initialized_.store(true);
    std::cout << "Application initialized successfully" << std::endl;
    return true;
}

bool Application::Start() {
    if (!initialized_.load()) {
        std::cerr << "Application not initialized" << std::endl;
        return false;
    }
    
    if (running_.load()) {
        std::cout << "Application already running" << std::endl;
        return true;
    }
    
    std::cout << "Starting application..." << std::endl;
    
    // 1. 启动工作线程
    StartWorkerThreads();
    
    // 2. 启动所有服务
    size_t started_services = service_registry_->StartAllServices();
    std::cout << "Started " << started_services << " services" << std::endl;
    
    // 3. 调用启动Hooks
    {
        std::lock_guard<std::mutex> lock(hooks_mutex_);
        for (auto& hook : startup_hooks_) {
            try {
                hook(*this);
            } catch (const std::exception& e) {
                std::cerr << "Startup hook error: " << e.what() << std::endl;
            }
        }
    }
    
    // 4. 启用服务健康检查
    service_registry_->SetAutoHealthCheck(true);
    
    running_.store(true);
    std::cout << "Application started successfully" << std::endl;
    return true;
}

void Application::Run() {
    if (!Start()) {
        std::cerr << "Failed to start application" << std::endl;
        return;
    }
    
    std::cout << "Application is running. Press Ctrl+C to stop." << std::endl;
    
    // 等待停止信号
    WaitForStop();
    
    std::cout << "Application stopped" << std::endl;
}

void Application::Stop() {
    if (!running_.load()) {
        return;
    }
    
    std::cout << "Stopping application..." << std::endl;
    stop_requested_.store(true);
    
    // 1. 调用停止Hooks
    CallShutdownHooks();
    
    // 2. 停止所有服务
    StopServices();
    
    // 3. 停止工作线程
    StopWorkerThreads();
    
    running_.store(false);
    
    // 通知等待的线程
    {
        std::lock_guard<std::mutex> lock(stop_mutex_);
        stop_condition_.notify_all();
    }
    
    std::cout << "Application stopped successfully" << std::endl;
}

void Application::WaitForStop() {
    std::unique_lock<std::mutex> lock(stop_mutex_);
    stop_condition_.wait(lock, [this] { return !running_.load(); });
}

bool Application::InitializeLogging() {
    try {
        const auto& logging_config = config_->GetLoggingConfig();
        
        // 使用Zeus日志配置系统
        auto& zeus_log_config = common::spdlog::ZeusLogConfig::Instance();
        zeus_log_config.SetGlobalLogDir(logging_config.log_dir);
        
        // 如果有自定义日志器配置，应用它们
        for (const auto& logger_config : logging_config.loggers) {
            // 这里可以添加具体的日志器配置逻辑
            std::cout << "Configured logger: " << logger_config.name << std::endl;
        }
        
        // 初始化默认控制台和文件日志
        if (logging_config.console || logging_config.file) {
            std::cout << "Logging system initialized" << std::endl;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing logging: " << e.what() << std::endl;
        return false;
    }
}

bool Application::InitializeNetworkModule() {
    try {
        // 初始化Zeus网络模块
        if (!common::network::NetworkModule::Initialize("", true)) {
            std::cerr << "Failed to initialize Zeus network module" << std::endl;
            return false;
        }
        
        std::cout << "Zeus Network Module v" << common::network::NetworkModule::GetVersion() << " initialized" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing network module: " << e.what() << std::endl;
        return false;
    }
}

bool Application::InitializeDependencyInjection() {
    try {
        // 注册配置提供者
        auto pg_provider = std::make_shared<config_providers::PostgreSQLConfigProvider>();
        auto redis_provider = std::make_shared<config_providers::RedisConfigProvider>();
        
        di_container_->RegisterConfigProvider<PostgreSQLConfig>("postgresql", pg_provider);
        di_container_->RegisterConfigProvider<RedisConfig>("redis", redis_provider);
        
        // 注册应用程序实例（单例）
        di_container_->RegisterSingleton<Application>(std::shared_ptr<Application>(&GetInstance(), [](Application*){}));
        
        // 注册其他组件
        di_container_->RegisterSingleton<AppConfig>(std::shared_ptr<AppConfig>(config_.get(), [](AppConfig*){}));
        di_container_->RegisterSingleton<ServiceRegistry>(std::shared_ptr<ServiceRegistry>(service_registry_.get(), [](ServiceRegistry*){}));
        di_container_->RegisterSingleton<ServiceFactory>(std::shared_ptr<ServiceFactory>(service_factory_.get(), [](ServiceFactory*){}));
        
        std::cout << "Dependency injection initialized" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing dependency injection: " << e.what() << std::endl;
        return false;
    }
}

bool Application::InitializeServices() {
    try {
        return CreateServicesFromConfig();
    } catch (const std::exception& e) {
        std::cerr << "Error initializing services: " << e.what() << std::endl;
        return false;
    }
}

bool Application::CreateServicesFromConfig() {
    bool success = true;
    
    // 创建监听器服务
    if (!CreateListenerServices()) {
        success = false;
    }
    
    // 创建连接器服务
    if (!CreateConnectorServices()) {
        success = false;
    }
    
    // 如果没有配置任何网络服务，创建默认服务
    if (config_->GetListenerConfigs().empty() && config_->GetConnectorConfigs().empty()) {
        std::cout << "No network services configured, creating default HTTP server on port 8080" << std::endl;
        
        ListenerConfig default_config;
        default_config.name = "default_http";
        default_config.type = "http";
        default_config.port = 8080;
        default_config.bind = "0.0.0.0";
        default_config.max_connections = 1000;
        
        HttpServiceOptions default_options;
        default_options.request_handler = [](const std::string& method, const std::string& path,
                                           const std::unordered_map<std::string, std::string>& headers,
                                           const std::string& body, std::string& response) {
            response = R"({"status": "ok", "message": "Zeus Application Server is running"})";
        };
        
        if (!CreateHttpService(default_config, default_options)) {
            std::cerr << "Failed to create default HTTP service" << std::endl;
            success = false;
        }
    }
    
    return success;
}

bool Application::CreateListenerServices() {
    const auto& listeners = config_->GetListenerConfigs();
    bool success = true;
    
    for (const auto& listener : listeners) {
        std::cout << "Creating listener service: " << listener.name << " (" << listener.type << ")" << std::endl;
        
        // 这里应该根据具体需求创建服务
        // 为了演示，我们创建基本的服务配置
        if (listener.type == "http" || listener.type == "https") {
            HttpServiceOptions options;
            options.request_handler = [listener](const std::string& method, const std::string& path,
                                               const std::unordered_map<std::string, std::string>& headers,
                                               const std::string& body, std::string& response) {
                response = R"({"service": ")" + listener.name + R"(", "status": "ok"})";
            };
            
            if (!CreateHttpService(listener, options)) {
                std::cerr << "Failed to create HTTP service: " << listener.name << std::endl;
                success = false;
            }
        } else if (listener.type == "tcp") {
            TcpServiceOptions options;
            options.on_connection = [listener](const std::string& conn_id, const std::string& endpoint) {
                std::cout << "TCP connection established [" << listener.name << "]: " << conn_id << " from " << endpoint << std::endl;
            };
            
            options.on_disconnect = [listener](const std::string& conn_id, boost::system::error_code ec) {
                std::cout << "TCP connection closed [" << listener.name << "]: " << conn_id << std::endl;
            };
            
            if (!CreateTcpService(listener, options)) {
                std::cerr << "Failed to create TCP service: " << listener.name << std::endl;
                success = false;
            }
        } else if (listener.type == "kcp") {
            KcpServiceOptions options;
            options.on_connection = [listener](const std::string& conn_id, const std::string& endpoint) {
                std::cout << "KCP connection established [" << listener.name << "]: " << conn_id << " from " << endpoint << std::endl;
            };
            
            if (!CreateKcpService(listener, options)) {
                std::cerr << "Failed to create KCP service: " << listener.name << std::endl;
                success = false;
            }
        }
    }
    
    return success;
}

bool Application::CreateConnectorServices() {
    const auto& connectors = config_->GetConnectorConfigs();
    bool success = true;
    
    for (const auto& connector : connectors) {
        std::cout << "Creating connector service: " << connector.name << " (" << connector.type << ")" << std::endl;
        
        // 连接器服务的创建逻辑
        // 这里可以根据需要实现具体的连接器创建逻辑
    }
    
    return success;
}

bool Application::CallInitHooks() {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    
    for (auto& hook : init_hooks_) {
        try {
            if (!hook(*this)) {
                std::cerr << "Init hook returned false" << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Init hook error: " << e.what() << std::endl;
            return false;
        }
    }
    
    return true;
}

void Application::StartWorkerThreads() {
    // 创建工作守护者，防止io_context提前退出
    work_guard_ = std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
        boost::asio::make_work_guard(io_context_)
    );
    
    // 启动工作线程
    worker_threads_.reserve(worker_thread_count_);
    for (size_t i = 0; i < worker_thread_count_; ++i) {
        worker_threads_.emplace_back(&Application::WorkerThreadFunction, this);
    }
    
    std::cout << "Started " << worker_thread_count_ << " worker threads" << std::endl;
}

void Application::WorkerThreadFunction() {
    try {
        io_context_.run();
    } catch (const std::exception& e) {
        std::cerr << "Worker thread error: " << e.what() << std::endl;
    }
}

void Application::StopServices() {
    service_registry_->SetAutoHealthCheck(false);
    service_registry_->StopAllServices();
}

void Application::StopWorkerThreads() {
    // 取消工作守护者，允许io_context退出
    if (work_guard_) {
        work_guard_->reset();
    }
    
    // 停止io_context
    io_context_.stop();
    
    // 等待所有工作线程结束
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    worker_threads_.clear();
    std::cout << "All worker threads stopped" << std::endl;
}

void Application::CallShutdownHooks() {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    
    for (auto& hook : shutdown_hooks_) {
        try {
            hook(*this);
        } catch (const std::exception& e) {
            std::cerr << "Shutdown hook error: " << e.what() << std::endl;
        }
    }
}

void Application::SetupSignalHandlers() {
    // 清理旧的信号集合
    if (signal_set_) {
        signal_set_->cancel();
    }
    
    // 创建新的信号集合，使用配置中的信号列表
    signal_set_ = std::make_unique<boost::asio::signal_set>(io_context_);
    
    for (int signal : signal_config_.handled_signals) {
        signal_set_->add(signal);
        if (signal_config_.log_signal_events) {
            std::cout << "Registered signal handler for signal " << signal << std::endl;
        }
    }
    
    signal_set_->async_wait([this](const boost::system::error_code& ec, int signal) {
        OnSignalReceived(ec, signal);
    });
}

void Application::OnSignalReceived(const boost::system::error_code& ec, int signal) {
    if (ec) {
        if (signal_config_.log_signal_events) {
            std::cerr << "Signal handler error: " << ec.message() << std::endl;
        }
        return;
    }
    
    if (signal_config_.log_signal_events) {
        std::cout << "\nReceived signal " << signal;
    }
    
    bool continue_default_handling = true;
    
    // 根据策略处理信号
    switch (signal_config_.strategy) {
        case SignalHandlerStrategy::DEFAULT_ONLY:
            // 仅使用默认处理
            break;
            
        case SignalHandlerStrategy::HOOK_FIRST:
            // Hook优先，然后默认处理
            ProcessSignalHooks(signal);
            break;
            
        case SignalHandlerStrategy::HOOK_ONLY:
            // 仅使用Hook处理
            ProcessSignalHooks(signal);
            continue_default_handling = false;
            break;
            
        case SignalHandlerStrategy::HOOK_OVERRIDE:
            // Hook决定是否继续默认处理
            continue_default_handling = ProcessSignalHandlers(signal);
            break;
    }
    
    // 执行默认处理
    if (continue_default_handling) {
        ExecuteDefaultSignalHandler(signal);
    }
    
    // 重新注册信号处理器（除非应用正在关闭）
    if (running_.load()) {
        signal_set_->async_wait([this](const boost::system::error_code& ec, int signal) {
            OnSignalReceived(ec, signal);
        });
    }
}

void Application::RegisterInitHook(hooks::InitHook hook) {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    init_hooks_.push_back(hook);
}

void Application::RegisterStartupHook(hooks::StartupHook hook) {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    startup_hooks_.push_back(hook);
}

void Application::RegisterShutdownHook(hooks::ShutdownHook hook) {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    shutdown_hooks_.push_back(hook);
}

void Application::SetSignalHandlerConfig(const SignalHandlerConfig& config) {
    std::lock_guard<std::mutex> lock(signal_mutex_);
    signal_config_ = config;
    
    // 如果已初始化，重新设置信号处理器
    if (initialized_.load()) {
        SetupSignalHandlers();
    }
}

void Application::RegisterSignalHook(int signal, hooks::SignalHook hook) {
    std::lock_guard<std::mutex> lock(signal_mutex_);
    signal_hooks_[signal].push_back(hook);
    
    if (signal_config_.log_signal_events) {
        std::cout << "Registered signal hook for signal " << signal << std::endl;
    }
}

void Application::RegisterSignalHandler(int signal, hooks::SignalHandler handler) {
    std::lock_guard<std::mutex> lock(signal_mutex_);
    signal_handlers_[signal].push_back(handler);
    
    if (signal_config_.log_signal_events) {
        std::cout << "Registered signal handler for signal " << signal << std::endl;
    }
}

void Application::ClearSignalHandlers(int signal) {
    std::lock_guard<std::mutex> lock(signal_mutex_);
    signal_hooks_[signal].clear();
    signal_handlers_[signal].clear();
    
    if (signal_config_.log_signal_events) {
        std::cout << "Cleared signal handlers for signal " << signal << std::endl;
    }
}

bool Application::ProcessSignalHooks(int signal) {
    std::lock_guard<std::mutex> lock(signal_mutex_);
    
    auto it = signal_hooks_.find(signal);
    if (it != signal_hooks_.end()) {
        for (auto& hook : it->second) {
            try {
                hook(*this, signal);
            } catch (const std::exception& e) {
                std::cerr << "Signal hook error for signal " << signal << ": " << e.what() << std::endl;
            }
        }
    }
    
    return true; // Hook总是允许继续默认处理
}

bool Application::ProcessSignalHandlers(int signal) {
    std::lock_guard<std::mutex> lock(signal_mutex_);
    
    auto it = signal_handlers_.find(signal);
    if (it != signal_handlers_.end()) {
        for (auto& handler : it->second) {
            try {
                if (!handler(*this, signal)) {
                    return false; // 如果任何handler返回false，停止默认处理
                }
            } catch (const std::exception& e) {
                std::cerr << "Signal handler error for signal " << signal << ": " << e.what() << std::endl;
            }
        }
    }
    
    return true; // 所有handler都返回true或没有handler
}

void Application::ExecuteDefaultSignalHandler(int signal) {
    if (signal_config_.log_signal_events) {
        std::cout << ", executing default handler..." << std::endl;
    }
    
    // 默认处理逻辑
    if (signal == SIGINT || signal == SIGTERM) {
        if (signal_config_.graceful_shutdown) {
            std::cout << "Initiating graceful shutdown..." << std::endl;
            Stop();
        } else {
            std::cout << "Initiating immediate shutdown..." << std::endl;
            std::exit(signal == SIGINT ? 130 : 143);
        }
    } else {
        std::cout << "Received signal " << signal << " but no default handler defined" << std::endl;
    }
}

bool Application::CreateTcpService(const ListenerConfig& config, const TcpServiceOptions& options) {
    auto service = service_factory_->CreateTcpServer(config, options);
    if (service) {
        return service_registry_->RegisterService(std::move(service));
    }
    return false;
}

bool Application::CreateTcpService(const ConnectorConfig& config, const TcpServiceOptions& options) {
    // 连接器版本的TCP服务创建
    // 这里需要实现连接器逻辑
    return true; // 临时返回
}

bool Application::CreateHttpService(const ListenerConfig& config, const HttpServiceOptions& options) {
    std::unique_ptr<Service> service;
    
    if (config.type == "https") {
        service = service_factory_->CreateHttpsServer(config, options);
    } else {
        service = service_factory_->CreateHttpServer(config, options);
    }
    
    if (service) {
        return service_registry_->RegisterService(std::move(service));
    }
    return false;
}

bool Application::CreateHttpService(const ConnectorConfig& config, const HttpServiceOptions& options) {
    // 连接器版本的HTTP服务创建
    return true; // 临时返回
}

bool Application::CreateKcpService(const ListenerConfig& config, const KcpServiceOptions& options) {
    auto service = service_factory_->CreateKcpServer(config, options);
    if (service) {
        return service_registry_->RegisterService(std::move(service));
    }
    return false;
}

bool Application::CreateKcpService(const ConnectorConfig& config, const KcpServiceOptions& options) {
    // 连接器版本的KCP服务创建
    return true; // 临时返回
}

std::optional<PostgreSQLConfig> Application::GetPostgreSQLConfig() const {
    return config_->GetPostgreSQLConfig();
}

std::optional<RedisConfig> Application::GetRedisConfig() const {
    return config_->GetRedisConfig();
}

// ========================= 命令行参数解析实现 =========================

void Application::SetArgumentParserConfig(const ArgumentParserConfig& config) {
    std::lock_guard<std::mutex> lock(args_mutex_);
    arg_parser_config_ = config;
    
    // 初始化默认参数（如果启用）
    if (config.auto_add_help || config.auto_add_version) {
        InitializeDefaultArguments();
    }
}

void Application::AddArgumentDefinition(const ArgumentDefinition& definition) {
    std::lock_guard<std::mutex> lock(args_mutex_);
    arg_parser_config_.arguments.push_back(definition);
    
    // 建立短名到长名的映射
    if (!definition.short_name.empty() && !definition.long_name.empty()) {
        arg_name_mapping_[definition.short_name] = definition.long_name;
    }
}

void Application::AddArgument(const std::string& short_name, const std::string& long_name,
                            const std::string& description, bool requires_value,
                            const std::string& default_value) {
    ArgumentDefinition definition;
    definition.short_name = short_name;
    definition.long_name = long_name;
    definition.description = description;
    definition.requires_value = requires_value;
    definition.is_flag = !requires_value;
    definition.default_value = default_value;
    
    AddArgumentDefinition(definition);
}

void Application::AddFlag(const std::string& short_name, const std::string& long_name,
                        const std::string& description) {
    AddArgument(short_name, long_name, description, false, "false");
}

void Application::AddArgumentWithHandler(const std::string& short_name, const std::string& long_name,
                                       const std::string& description, hooks::ArgumentHandler handler,
                                       bool requires_value) {
    ArgumentDefinition definition;
    definition.short_name = short_name;
    definition.long_name = long_name;
    definition.description = description;
    definition.requires_value = requires_value;
    definition.is_flag = !requires_value;
    definition.handler = handler;
    
    AddArgumentDefinition(definition);
}

ParsedArguments Application::ParseArgs(int argc, char* argv[]) {
    std::lock_guard<std::mutex> lock(args_mutex_);
    
    parsed_args_ = ParsedArguments{}; // 重置解析结果
    
    if (argc <= 0 || !argv) {
        parsed_args_.error_message = "Invalid arguments provided";
        return parsed_args_;
    }
    
    // 设置程序名称
    if (arg_parser_config_.program_name.empty() && argc > 0) {
        arg_parser_config_.program_name = argv[0];
    }
    
    // 转换为字符串向量以便处理
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    
    // 解析参数
    for (int i = 0; i < static_cast<int>(args.size()); ++i) {
        const std::string& arg = args[i];
        
        if (arg.empty()) {
            continue;
        }
        
        // 检查是否为选项参数
        if (arg[0] == '-') {
            std::string next_arg = (i + 1 < static_cast<int>(args.size())) ? args[i + 1] : "";
            
            if (!ParseSingleArgument(arg, next_arg, i, args)) {
                return parsed_args_; // 解析失败
            }
        } else {
            // 位置参数
            parsed_args_.positional_args.push_back(arg);
        }
    }
    
    // 检查帮助和版本请求
    if (HasArgument("help") || HasArgument("h")) {
        parsed_args_.help_requested = true;
    }
    if (HasArgument("version") || HasArgument("v")) {
        parsed_args_.version_requested = true;
    }
    
    // 填充默认值
    for (const auto& def : arg_parser_config_.arguments) {
        const std::string& name = def.long_name.empty() ? def.short_name : def.long_name;
        if (!HasArgument(name) && !def.default_value.empty()) {
            parsed_args_.values[name] = def.default_value;
        }
    }
    
    return parsed_args_;
}

std::string Application::GetArgumentValue(const std::string& name, const std::string& default_value) const {
    std::lock_guard<std::mutex> lock(args_mutex_);
    
    std::string resolved_name = ResolveArgumentName(name);
    
    auto it = parsed_args_.values.find(resolved_name);
    if (it != parsed_args_.values.end()) {
        return it->second;
    }
    
    // 尝试使用原始名称
    it = parsed_args_.values.find(name);
    if (it != parsed_args_.values.end()) {
        return it->second;
    }
    
    return default_value;
}

bool Application::HasArgument(const std::string& name) const {
    std::lock_guard<std::mutex> lock(args_mutex_);
    
    std::string resolved_name = ResolveArgumentName(name);
    
    return parsed_args_.values.find(resolved_name) != parsed_args_.values.end() ||
           parsed_args_.values.find(name) != parsed_args_.values.end();
}

void Application::SetUsageProvider(hooks::UsageProvider provider) {
    std::lock_guard<std::mutex> lock(args_mutex_);
    arg_parser_config_.usage_provider = provider;
}

void Application::SetVersionProvider(hooks::VersionProvider provider) {
    std::lock_guard<std::mutex> lock(args_mutex_);
    arg_parser_config_.version_provider = provider;
}

void Application::ShowUsage(const std::string& program_name) const {
    if (arg_parser_config_.usage_provider) {
        arg_parser_config_.usage_provider(program_name);
    } else {
        ShowDefaultUsage(program_name);
    }
}

void Application::ShowVersion() const {
    if (arg_parser_config_.version_provider) {
        arg_parser_config_.version_provider();
    } else {
        ShowDefaultVersion();
    }
}

// ========================= 私有方法实现 =========================

bool Application::ParseSingleArgument(const std::string& arg, const std::string& next_arg,
                                     int& arg_index, const std::vector<std::string>& args) {
    std::string arg_name;
    std::string arg_value;
    bool has_value = false;
    
    // 解析参数格式
    if (arg.substr(0, 2) == "--") {
        // 长参数格式: --config=value 或 --config value
        size_t eq_pos = arg.find('=');
        if (eq_pos != std::string::npos) {
            arg_name = arg.substr(2, eq_pos - 2);
            arg_value = arg.substr(eq_pos + 1);
            has_value = true;
        } else {
            arg_name = arg.substr(2);
        }
    } else if (arg.length() > 1 && arg[0] == '-') {
        // 短参数格式: -c value
        arg_name = arg.substr(1);
    } else {
        parsed_args_.error_message = "Invalid argument format: " + arg;
        return false;
    }
    
    // 查找参数定义
    ArgumentDefinition* def = FindArgumentDefinition(arg_name);
    if (!def) {
        parsed_args_.error_message = "Unknown argument: " + arg;
        return false;
    }
    
    // 处理参数值
    if (def->requires_value && !has_value) {
        if (next_arg.empty() || next_arg[0] == '-') {
            parsed_args_.error_message = "Argument --" + def->long_name + " requires a value";
            return false;
        }
        arg_value = next_arg;
        arg_index++; // 跳过下一个参数（已用作当前参数的值）
        has_value = true;
    } else if (def->is_flag) {
        arg_value = "true";
        has_value = true;
    }
    
    // 存储参数值
    std::string store_name = def->long_name.empty() ? def->short_name : def->long_name;
    if (has_value) {
        parsed_args_.values[store_name] = arg_value;
    }
    
    // 调用自定义处理器
    if (def->handler) {
        try {
            if (!def->handler(*this, store_name, arg_value)) {
                parsed_args_.error_message = "Handler failed for argument: " + store_name;
                return false;
            }
        } catch (const std::exception& e) {
            parsed_args_.error_message = "Handler error for argument " + store_name + ": " + e.what();
            return false;
        }
    }
    
    return true;
}

ArgumentDefinition* Application::FindArgumentDefinition(const std::string& name) {
    for (auto& def : arg_parser_config_.arguments) {
        if (def.short_name == name || def.long_name == name) {
            return &def;
        }
    }
    return nullptr;
}

std::string Application::ResolveArgumentName(const std::string& name) const {
    auto it = arg_name_mapping_.find(name);
    return (it != arg_name_mapping_.end()) ? it->second : name;
}

void Application::ShowDefaultUsage(const std::string& program_name) const {
    std::cout << "Zeus Application Framework" << std::endl;
    std::cout << "Usage: " << program_name << " [options]" << std::endl;
    
    if (!arg_parser_config_.program_description.empty()) {
        std::cout << "\nDescription: " << arg_parser_config_.program_description << std::endl;
    }
    
    if (!arg_parser_config_.arguments.empty()) {
        std::cout << "\nOptions:" << std::endl;
        
        for (const auto& def : arg_parser_config_.arguments) {
            std::string short_form = def.short_name.empty() ? "" : "-" + def.short_name;
            std::string long_form = def.long_name.empty() ? "" : "--" + def.long_name;
            
            std::string option_text;
            if (!short_form.empty() && !long_form.empty()) {
                option_text = short_form + ", " + long_form;
            } else if (!short_form.empty()) {
                option_text = short_form;
            } else if (!long_form.empty()) {
                option_text = long_form;
            }
            
            if (def.requires_value) {
                option_text += " <value>";
            }
            
            std::cout << "  " << std::left << std::setw(25) << option_text << def.description;
            
            if (!def.default_value.empty() && def.requires_value) {
                std::cout << " (default: " << def.default_value << ")";
            }
            
            std::cout << std::endl;
        }
    }
    
    std::cout << std::endl;
}

void Application::ShowDefaultVersion() const {
    std::cout << "Zeus Application Framework" << std::endl;
    if (!arg_parser_config_.program_version.empty()) {
        std::cout << "Version: " << arg_parser_config_.program_version << std::endl;
    }
    std::cout << "Build: " << __DATE__ << " " << __TIME__ << std::endl;
}

void Application::InitializeDefaultArguments() {
    if (arg_parser_config_.auto_add_help) {
        ArgumentDefinition help_def;
        help_def.short_name = "h";
        help_def.long_name = "help";
        help_def.description = "Show this help message";
        help_def.is_flag = true;
        help_def.requires_value = false;
        arg_parser_config_.arguments.push_back(help_def);
        arg_name_mapping_["h"] = "help";
    }
    
    if (arg_parser_config_.auto_add_version) {
        ArgumentDefinition version_def;
        version_def.short_name = "v";
        version_def.long_name = "version";
        version_def.description = "Show version information";
        version_def.is_flag = true;
        version_def.requires_value = false;
        arg_parser_config_.arguments.push_back(version_def);
        arg_name_mapping_["v"] = "version";
    }
}

// ========================= ListenEndpoint 实现 =========================

std::optional<hooks::ListenEndpoint> hooks::ListenEndpoint::Parse(const std::string& str) {
    // 格式: protocol://address:port 或 address:port (默认tcp)
    ListenEndpoint endpoint;
    
    std::string remaining = str;
    
    // 解析协议
    size_t proto_pos = remaining.find("://");
    if (proto_pos != std::string::npos) {
        endpoint.protocol = remaining.substr(0, proto_pos);
        remaining = remaining.substr(proto_pos + 3);
    } else {
        endpoint.protocol = "tcp"; // 默认协议
    }
    
    // 验证协议
    static const std::vector<std::string> valid_protocols = {"tcp", "kcp", "http", "https"};
    if (std::find(valid_protocols.begin(), valid_protocols.end(), endpoint.protocol) == valid_protocols.end()) {
        return std::nullopt;
    }
    
    // 解析地址和端口
    size_t colon_pos = remaining.rfind(':');
    if (colon_pos == std::string::npos) {
        return std::nullopt;
    }
    
    endpoint.address = remaining.substr(0, colon_pos);
    std::string port_str = remaining.substr(colon_pos + 1);
    
    try {
        int port = std::stoi(port_str);
        if (port <= 0 || port > 65535) {
            return std::nullopt;
        }
        endpoint.port = static_cast<uint16_t>(port);
    } catch (const std::exception&) {
        return std::nullopt;
    }
    
    return endpoint;
}

// ========================= 命令行配置覆盖实现 =========================

hooks::CommandLineOverrides Application::GetCommandLineOverrides() const {
    std::lock_guard<std::mutex> lock(args_mutex_);
    
    hooks::CommandLineOverrides overrides;
    
    // 注意：当前的parsed_args_.values是map类型，不支持多值
    // 这里需要修改为从实际的多值存储中获取
    // 暂时实现单值版本，后续可以扩展为多值支持
    
    auto it = parsed_args_.values.find("listen");
    if (it != parsed_args_.values.end()) {
        auto endpoint = hooks::ListenEndpoint::Parse(it->second);
        if (endpoint) {
            overrides.listen_endpoints.push_back(*endpoint);
        }
    }
    
    it = parsed_args_.values.find("backend");
    if (it != parsed_args_.values.end()) {
        overrides.backend_servers.push_back(it->second);
    }
    
    it = parsed_args_.values.find("log-level");
    if (it != parsed_args_.values.end()) {
        overrides.log_level = it->second;
    }
    
    it = parsed_args_.values.find("max-connections");
    if (it != parsed_args_.values.end()) {
        try {
            overrides.max_connections = std::stoul(it->second);
        } catch (const std::exception&) {
            // 忽略无效值
        }
    }
    
    it = parsed_args_.values.find("timeout");
    if (it != parsed_args_.values.end()) {
        try {
            overrides.timeout_ms = std::stoul(it->second);
        } catch (const std::exception&) {
            // 忽略无效值
        }
    }
    
    if (HasArgument("daemon")) {
        overrides.daemon_mode = true;
    }
    
    return overrides;
}

bool Application::HasCommandLineOverrides() const {
    return GetCommandLineOverrides().HasOverrides();
}

bool Application::IntegrateCommandLineOverrides() {
    auto overrides = GetCommandLineOverrides();
    
    // 处理listen_endpoints
    for (const auto& endpoint : overrides.listen_endpoints) {
        // 检查端口冲突
        if (config_->HasPortConflict(endpoint.port, endpoint.address)) {
            std::cerr << "❌ 端口冲突: " << endpoint.address << ":" << endpoint.port << std::endl;
            return false;  // 终止程序
        }
        
        // 转换为ListenerConfig
        ListenerConfig listener_config;
        listener_config.name = "cmdline_" + endpoint.protocol + "_" + std::to_string(endpoint.port);
        listener_config.type = endpoint.protocol;
        listener_config.port = endpoint.port;
        listener_config.bind = endpoint.address;
        
        // 从配置文件中查找相同协议的配置，复制缺失参数
        CopyDefaultOptionsFromConfig(listener_config);
        
        config_->AddListenerConfig(listener_config);
        
        std::cout << "✅ 添加命令行监听服务: " << endpoint.protocol << "://" 
                  << endpoint.address << ":" << endpoint.port << std::endl;
    }
    
    // 处理backend_servers
    for (const auto& backend : overrides.backend_servers) {
        ConnectorConfig connector_config;
        connector_config.name = "cmdline_backend_" + backend;
        connector_config.type = "tcp"; // 默认协议
        connector_config.targets = {backend};
        
        CopyDefaultOptionsFromConfig(connector_config);
        config_->AddConnectorConfig(connector_config);
        
        std::cout << "✅ 添加命令行后端连接: " << backend << std::endl;
    }
    
    return true;
}

void Application::CopyDefaultOptionsFromConfig(ListenerConfig& config) const {
    // 查找配置文件中相同类型的监听器，直接复制所有参数
    for (const auto& existing : config_->GetListenerConfigs()) {
        if (existing.type == config.type) {
            // 直接赋值，无需任何判断
            config.max_connections = existing.max_connections;
            config.options = existing.options;  // 复制协议特定选项（如缓冲区大小）
            config.ssl = existing.ssl;          // 复制SSL配置
            std::cout << "📋 从配置文件继承" << config.type << "协议参数" << std::endl;
            break;
        }
    }
}

void Application::CopyDefaultOptionsFromConfig(ConnectorConfig& config) const {
    // 查找配置文件中相同类型的连接器，直接复制所有参数
    for (const auto& existing : config_->GetConnectorConfigs()) {
        if (existing.type == config.type) {
            config.options = existing.options;  // 复制协议特定选项
            config.ssl = existing.ssl;          // 复制SSL配置
            std::cout << "📋 从配置文件继承" << config.type << "连接器参数" << std::endl;
            break;
        }
    }
}

} // namespace app
} // namespace core