/**
 * @file gateway_with_enhanced_app.cpp
 * @brief 使用增强Application框架的Gateway示例
 * 
 * 演示如何使用Zeus Application框架的命令行参数解析和信号处理功能
 */

#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <algorithm>

// 模拟Gateway相关类型和函数
namespace gateway_demo {

struct GatewayConfig {
    uint16_t listen_port = 8080;
    std::string bind_address = "0.0.0.0";
    std::vector<std::string> backend_servers;
    uint32_t max_client_connections = 10000;
    uint32_t max_backend_connections = 100;
    uint32_t client_timeout_ms = 60000;
    uint32_t backend_timeout_ms = 30000;
    uint32_t heartbeat_interval_ms = 30000;
};

struct GatewayStats {
    size_t total_sessions_created = 0;
    size_t active_sessions = 0;
    uint64_t bytes_transferred = 0;
    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
};

class MockGatewayServer {
public:
    MockGatewayServer(const GatewayConfig& config) : config_(config), running_(false) {}
    
    bool Start() {
        if (running_.load()) return true;
        
        std::cout << "🚀 Starting Gateway Server..." << std::endl;
        std::cout << "   Listening on " << config_.bind_address << ":" << config_.listen_port << std::endl;
        std::cout << "   Backend servers: " << config_.backend_servers.size() << std::endl;
        
        running_.store(true);
        stats_.start_time = std::chrono::steady_clock::now();
        
        // 模拟一些初始统计数据
        stats_.total_sessions_created = 0;
        stats_.active_sessions = 0;
        
        return true;
    }
    
    void Stop() {
        if (!running_.load()) return;
        
        std::cout << "🛑 Stopping Gateway Server..." << std::endl;
        running_.store(false);
    }
    
    bool IsRunning() const { return running_.load(); }
    
    const GatewayStats& GetStats() const { return stats_; }
    
    void UpdateStats() {
        // 模拟统计数据更新
        static int counter = 0;
        counter++;
        
        stats_.active_sessions = (counter % 10) + 1; // 1-10 活跃会话
        stats_.total_sessions_created += (counter % 3); // 随机增加
        stats_.bytes_transferred += (counter % 1000) * 1024; // 随机字节数
    }

private:
    GatewayConfig config_;
    GatewayStats stats_;
    std::atomic<bool> running_;
};

// 模拟Application框架的简化版本
class MockApplication {
public:
    // 命令行参数相关类型
    using ArgumentHandler = std::function<bool(MockApplication&, const std::string&, const std::string&)>;
    using UsageProvider = std::function<void(const std::string&)>;
    using VersionProvider = std::function<void()>;
    
    struct ArgumentDefinition {
        std::string short_name;
        std::string long_name;
        std::string description;
        bool requires_value = false;
        bool is_flag = false;
        std::string default_value;
        ArgumentHandler handler;
    };
    
    struct ParsedArguments {
        std::unordered_map<std::string, std::string> values;
        std::vector<std::string> positional_args;
        bool help_requested = false;
        bool version_requested = false;
        std::string error_message;
    };
    
    MockApplication() = default;
    
    // 添加参数定义
    void AddArgument(const std::string& short_name, const std::string& long_name,
                    const std::string& description, bool requires_value = true,
                    const std::string& default_value = "") {
        ArgumentDefinition def;
        def.short_name = short_name;
        def.long_name = long_name;
        def.description = description;
        def.requires_value = requires_value;
        def.is_flag = !requires_value;
        def.default_value = default_value;
        arguments_.push_back(def);
    }
    
    void AddArgumentWithHandler(const std::string& short_name, const std::string& long_name,
                              const std::string& description, ArgumentHandler handler,
                              bool requires_value = true) {
        ArgumentDefinition def;
        def.short_name = short_name;
        def.long_name = long_name;
        def.description = description;
        def.requires_value = requires_value;
        def.is_flag = !requires_value;
        def.handler = handler;
        arguments_.push_back(def);
    }
    
    // 解析命令行参数
    ParsedArguments ParseArgs(int argc, char* argv[]) {
        parsed_args_ = ParsedArguments{};
        
        if (argc <= 0 || !argv) {
            parsed_args_.error_message = "Invalid arguments";
            return parsed_args_;
        }
        
        program_name_ = argv[0];
        
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            if (arg == "-h" || arg == "--help") {
                parsed_args_.help_requested = true;
            } else if (arg == "-v" || arg == "--version") {
                parsed_args_.version_requested = true;
            } else if (arg.substr(0, 2) == "--") {
                // 长参数格式
                size_t eq_pos = arg.find('=');
                std::string name, value;
                
                if (eq_pos != std::string::npos) {
                    name = arg.substr(2, eq_pos - 2);
                    value = arg.substr(eq_pos + 1);
                } else {
                    name = arg.substr(2);
                    if (i + 1 < argc && argv[i + 1][0] != '-') {
                        value = argv[++i];
                    }
                }
                
                parsed_args_.values[name] = value;
            } else if (arg[0] == '-' && arg.length() > 1) {
                // 短参数格式
                std::string name = arg.substr(1);
                std::string value;
                
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    value = argv[++i];
                }
                
                parsed_args_.values[name] = value;
            } else {
                // 位置参数
                parsed_args_.positional_args.push_back(arg);
            }
        }
        
        return parsed_args_;
    }
    
    std::string GetArgumentValue(const std::string& name, const std::string& default_value = "") const {
        auto it = parsed_args_.values.find(name);
        if (it != parsed_args_.values.end()) {
            return it->second;
        }
        return default_value;
    }
    
    bool HasArgument(const std::string& name) const {
        return parsed_args_.values.find(name) != parsed_args_.values.end();
    }
    
    void SetUsageProvider(UsageProvider provider) {
        usage_provider_ = provider;
    }
    
    void SetVersionProvider(VersionProvider provider) {
        version_provider_ = provider;
    }
    
    void ShowUsage() const {
        if (usage_provider_) {
            usage_provider_(program_name_);
        } else {
            ShowDefaultUsage();
        }
    }
    
    void ShowVersion() const {
        if (version_provider_) {
            version_provider_();
        } else {
            ShowDefaultVersion();
        }
    }
    
    const ParsedArguments& GetParsedArguments() const { return parsed_args_; }

private:
    void ShowDefaultUsage() const {
        std::cout << "Zeus Gateway Server with Enhanced Application Framework" << std::endl;
        std::cout << "Usage: " << program_name_ << " [options]" << std::endl;
        std::cout << "\nOptions:" << std::endl;
        
        // 显示定义的参数
        for (const auto& def : arguments_) {
            std::string short_form = def.short_name.empty() ? "" : "-" + def.short_name;
            std::string long_form = def.long_name.empty() ? "" : "--" + def.long_name;
            
            std::string option_text;
            if (!short_form.empty() && !long_form.empty()) {
                option_text = short_form + ", " + long_form;
            } else if (!short_form.empty()) {
                option_text = short_form;
            } else {
                option_text = long_form;
            }
            
            if (def.requires_value) {
                option_text += " <value>";
            }
            
            std::cout << "  " << std::left << std::setw(25) << option_text << def.description;
            
            if (!def.default_value.empty()) {
                std::cout << " (default: " << def.default_value << ")";
            }
            
            std::cout << std::endl;
        }
        
        // 默认参数
        std::cout << "  -h, --help               Show this help message" << std::endl;
        std::cout << "  -v, --version            Show version information" << std::endl;
        std::cout << std::endl;
    }
    
    void ShowDefaultVersion() const {
        std::cout << "Zeus Gateway Server" << std::endl;
        std::cout << "Version: 1.0.0 (Enhanced)" << std::endl;
        std::cout << "Build: " << __DATE__ << " " << __TIME__ << std::endl;
#ifdef ZEUS_USE_KCP
        std::cout << "Protocol: KCP" << std::endl;
#else
        std::cout << "Protocol: TCP" << std::endl;
#endif
    }
    
    std::vector<ArgumentDefinition> arguments_;
    ParsedArguments parsed_args_;
    std::string program_name_;
    UsageProvider usage_provider_;
    VersionProvider version_provider_;
};

} // namespace gateway_demo

using namespace gateway_demo;

// 全局Gateway实例
std::unique_ptr<MockGatewayServer> g_gateway_server;
std::atomic<bool> g_running{true};

/**
 * @brief 自定义使用帮助显示
 */
void ShowGatewayUsage(const std::string& program_name) {
    std::cout << "🌉 Zeus Gateway Server v1.0.0 (Enhanced Framework Demo)" << std::endl;
    std::cout << "用法: " << program_name << " [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -c, --config <文件>      指定配置文件路径" << std::endl;
    std::cout << "  -p, --port <端口>        指定监听端口 (默认: 8080)" << std::endl;
    std::cout << "  -b, --bind <地址>        指定绑定地址 (默认: 0.0.0.0)" << std::endl;
    std::cout << "  -d, --daemon             后台运行模式" << std::endl;
    std::cout << "  -l, --log-level <级别>   设置日志级别 (debug|info|warn|error)" << std::endl;
    std::cout << "  -h, --help               显示此帮助信息" << std::endl;
    std::cout << "  -v, --version            显示版本信息" << std::endl;
    std::cout << std::endl;
    std::cout << "示例:" << std::endl;
    std::cout << "  " << program_name << "                          # 使用默认配置" << std::endl;
    std::cout << "  " << program_name << " -c gateway.json            # 指定配置文件" << std::endl;
    std::cout << "  " << program_name << " -p 9090 -b 127.0.0.1      # 自定义端口和地址" << std::endl;
    std::cout << "  " << program_name << " -d -l info                 # 后台运行，info级别日志" << std::endl;
    std::cout << std::endl;
    std::cout << "信号处理:" << std::endl;
    std::cout << "  SIGINT (Ctrl+C)    - 优雅关闭" << std::endl;
    std::cout << "  SIGTERM            - 终止服务" << std::endl;
    std::cout << "  SIGUSR1            - 重载配置" << std::endl;
    std::cout << "  SIGUSR2            - 显示统计信息" << std::endl;
}

/**
 * @brief 自定义版本信息显示
 */
void ShowGatewayVersion() {
    std::cout << "🌉 Zeus Gateway Server" << std::endl;
    std::cout << "版本: 1.0.0 (Enhanced Framework Demo)" << std::endl;
    std::cout << "构建时间: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << "框架: Zeus Application Framework v2.0" << std::endl;
#ifdef ZEUS_USE_KCP
    std::cout << "协议: KCP (高性能UDP)" << std::endl;
#else
    std::cout << "协议: TCP (可靠传输)" << std::endl;
#endif
    std::cout << "功能特性:" << std::endl;
    std::cout << "  ✅ 增强命令行参数解析" << std::endl;
    std::cout << "  ✅ 灵活信号处理机制" << std::endl;
    std::cout << "  ✅ 自定义Usage和Version显示" << std::endl;
    std::cout << "  ✅ 负载均衡和会话管理" << std::endl;
}

/**
 * @brief 加载Gateway配置
 */
bool LoadGatewayConfig(const std::string& config_file, GatewayConfig& config) {
    // 简化版配置加载 - 仅检查文件是否存在
    std::ifstream file(config_file);
    if (!file.is_open()) {
        return false;
    }
    
    // 模拟配置加载成功
    config.listen_port = 8080;
    config.bind_address = "0.0.0.0";
    config.backend_servers = {"127.0.0.1:8081", "127.0.0.1:8082", "127.0.0.1:8083"};
    config.max_client_connections = 10000;
    config.max_backend_connections = 100;
    
    return true;
}

/**
 * @brief 信号处理函数
 */
void SignalHandler(int signal) {
    switch (signal) {
        case SIGINT:
            std::cout << "\n🛑 收到SIGINT信号，正在优雅关闭..." << std::endl;
            g_running = false;
            break;
        case SIGTERM:
            std::cout << "\n⚡ 收到SIGTERM信号，正在强制关闭..." << std::endl;
            g_running = false;
            break;
        case SIGUSR1:
            std::cout << "\n🔄 收到SIGUSR1信号，重载配置..." << std::endl;
            // 这里可以实现配置重载逻辑
            break;
        case SIGUSR2:
            std::cout << "\n📊 收到SIGUSR2信号，显示统计信息:" << std::endl;
            if (g_gateway_server) {
                const auto& stats = g_gateway_server->GetStats();
                auto now = std::chrono::steady_clock::now();
                auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - stats.start_time).count();
                
                std::cout << "  运行时间: " << uptime << " 秒" << std::endl;
                std::cout << "  总创建会话: " << stats.total_sessions_created << std::endl;
                std::cout << "  活跃会话: " << stats.active_sessions << std::endl;
                std::cout << "  传输字节数: " << stats.bytes_transferred << " bytes" << std::endl;
            }
            break;
    }
}

/**
 * @brief 设置自定义参数处理
 */
void SetupArgumentHandlers(MockApplication& app) {
    // 配置文件参数
    app.AddArgument("c", "config", "指定配置文件路径", true, "config/gateway/gateway.json.default");
    
    // 端口参数 - 带自定义处理器
    app.AddArgumentWithHandler("p", "port", "指定监听端口", 
        [](MockApplication& app, const std::string& name, const std::string& value) -> bool {
            try {
                int port = std::stoi(value);
                if (port <= 0 || port > 65535) {
                    std::cerr << "错误: 端口号必须在1-65535范围内" << std::endl;
                    return false;
                }
                std::cout << "✅ 端口设置为: " << port << std::endl;
                return true;
            } catch (const std::exception&) {
                std::cerr << "错误: 无效的端口号: " << value << std::endl;
                return false;
            }
        }, true);
    
    // 绑定地址参数
    app.AddArgument("b", "bind", "指定绑定地址", true, "0.0.0.0");
    
    // 后台运行标志
    app.AddArgument("d", "daemon", "后台运行模式", false);
    
    // 日志级别参数 - 带验证处理器
    app.AddArgumentWithHandler("l", "log-level", "设置日志级别 (debug|info|warn|error)",
        [](MockApplication& app, const std::string& name, const std::string& value) -> bool {
            static const std::vector<std::string> valid_levels = {"debug", "info", "warn", "error"};
            
            if (std::find(valid_levels.begin(), valid_levels.end(), value) == valid_levels.end()) {
                std::cerr << "错误: 无效的日志级别: " << value << std::endl;
                std::cerr << "支持的级别: debug, info, warn, error" << std::endl;
                return false;
            }
            
            std::cout << "✅ 日志级别设置为: " << value << std::endl;
            return true;
        }, true);
    
    // 设置自定义Usage和Version显示器
    app.SetUsageProvider(ShowGatewayUsage);
    app.SetVersionProvider(ShowGatewayVersion);
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    try {
        MockApplication app;
        
        // 设置参数处理
        SetupArgumentHandlers(app);
        
        // 解析命令行参数
        auto parsed_args = app.ParseArgs(argc, argv);
        
        // 检查解析错误
        if (!parsed_args.error_message.empty()) {
            std::cerr << "❌ 参数解析错误: " << parsed_args.error_message << std::endl;
            app.ShowUsage();
            return 1;
        }
        
        // 处理帮助请求
        if (parsed_args.help_requested) {
            app.ShowUsage();
            return 0;
        }
        
        // 处理版本请求
        if (parsed_args.version_requested) {
            app.ShowVersion();
            return 0;
        }
        
        std::cout << "\n🎯 === Gateway启动参数解析演示 ===" << std::endl;
        
        // 显示解析结果
        std::cout << "\n📝 解析到的参数:" << std::endl;
        for (const auto& [key, value] : parsed_args.values) {
            std::cout << "  " << key << " = " << value << std::endl;
        }
        
        if (!parsed_args.positional_args.empty()) {
            std::cout << "\n📍 位置参数:" << std::endl;
            for (const auto& arg : parsed_args.positional_args) {
                std::cout << "  " << arg << std::endl;
            }
        }
        
        // 创建Gateway配置
        GatewayConfig config;
        
        // 从命令行参数覆盖配置
        if (app.HasArgument("port")) {
            config.listen_port = static_cast<uint16_t>(std::stoi(app.GetArgumentValue("port", "8080")));
        }
        
        if (app.HasArgument("bind")) {
            config.bind_address = app.GetArgumentValue("bind", "0.0.0.0");
        }
        
        // 尝试加载配置文件
        std::string config_file = app.GetArgumentValue("config", "config/gateway/gateway.json.default");
        if (!LoadGatewayConfig(config_file, config)) {
            std::cout << "⚠️  配置文件加载失败，使用默认配置: " << config_file << std::endl;
            
            // 设置默认后端服务器
            config.backend_servers = {"127.0.0.1:8081", "127.0.0.1:8082", "127.0.0.1:8083"};
        }
        
        // 创建Gateway服务器
        g_gateway_server = std::make_unique<MockGatewayServer>(config);
        
        // 设置信号处理
        std::signal(SIGINT, SignalHandler);
        std::signal(SIGTERM, SignalHandler);
        std::signal(SIGUSR1, SignalHandler);
        std::signal(SIGUSR2, SignalHandler);
        
        // 启动Gateway
        if (!g_gateway_server->Start()) {
            std::cerr << "❌ Gateway启动失败" << std::endl;
            return 1;
        }
        
        std::cout << "\n🎯 === Gateway服务运行中 ===" << std::endl;
        std::cout << "💡 提示:" << std::endl;
        std::cout << "  - Ctrl+C (SIGINT) 优雅关闭" << std::endl;
        std::cout << "  - kill -TERM <pid> 强制关闭" << std::endl;
        std::cout << "  - kill -USR1 <pid> 重载配置" << std::endl;
        std::cout << "  - kill -USR2 <pid> 显示统计" << std::endl;
        
        if (app.HasArgument("daemon")) {
            std::cout << "🌙 后台运行模式已启用" << std::endl;
        }
        
        std::cout << "================================\n" << std::endl;
        
        // 主循环
        int counter = 0;
        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            if (g_gateway_server) {
                g_gateway_server->UpdateStats();
                
                counter++;
                if (counter % 10 == 0) { // 每20秒显示一次状态
                    const auto& stats = g_gateway_server->GetStats();
                    std::cout << "📊 状态更新 - 活跃会话: " << stats.active_sessions 
                              << ", 总会话: " << stats.total_sessions_created << std::endl;
                }
            }
        }
        
        // 关闭Gateway
        if (g_gateway_server) {
            const auto& stats = g_gateway_server->GetStats();
            std::cout << "\n📊 最终统计:" << std::endl;
            std::cout << "  总创建会话: " << stats.total_sessions_created << std::endl;
            std::cout << "  活跃会话: " << stats.active_sessions << std::endl;
            std::cout << "  传输字节数: " << stats.bytes_transferred << " bytes" << std::endl;
            
            g_gateway_server->Stop();
            g_gateway_server.reset();
        }
        
        std::cout << "\n✅ Gateway正常退出" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 应用程序错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 未知应用程序错误" << std::endl;
        return 1;
    }
}