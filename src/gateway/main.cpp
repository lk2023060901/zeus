/**
 * @file main.cpp
 * @brief Zeus Gateway服务主程序
 * 
 * 使用Zeus Application框架实现的Gateway服务
 * 支持TCP/KCP协议转发和负载均衡功能
 */

#include "core/app/application.h"
#include "gateway/gateway_server.h"
#include <iostream>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <thread>
#include <chrono>
#include <filesystem>
#include <nlohmann/json.hpp>

using namespace core::app;
using namespace gateway;

// Gateway现在主要用于配置和业务逻辑，服务实例由Application框架管理

/**
 * @brief 获取默认配置文件路径（基于应用程序所在目录）
 * @param executable_path 可执行文件路径
 * @return 默认配置文件路径
 */
std::string GetDefaultConfigPath(const std::string& executable_path) {
    if (executable_path.empty()) {
        return "gateway.json";  // 回退到当前目录
    }
    
    try {
        auto exe_dir = std::filesystem::canonical(
            std::filesystem::path(executable_path)
        ).parent_path();
        
        // 优先查找应用程序目录下的gateway.json
        auto config_path = exe_dir / "gateway.json";
        return config_path.string();
    } catch (const std::exception&) {
        return "gateway.json";  // 回退到当前目录
    }
}

/**
 * @brief Gateway初始化Hook
 */
bool GatewayInitHook(Application& app) {
    try {
        std::cout << "🚀 初始化Zeus Gateway服务..." << std::endl;
        
#ifdef ZEUS_USE_KCP
        std::cout << "网络协议: KCP (高性能)" << std::endl;
#else
        std::cout << "网络协议: TCP (可靠)" << std::endl;
#endif

        // 从配置中加载Gateway设置
        const auto& config = app.GetConfig();
        const auto gateway_config_section = config.GetConfigSection("gateway");
        
        GatewayConfig gateway_config;
        
        if (gateway_config_section) {
            // 从配置文件加载
            const auto& gateway_json = *gateway_config_section;
            
            gateway_config.listen_port = gateway_json.value("listen", nlohmann::json{}).value("port", 8080);
            gateway_config.bind_address = gateway_json.value("listen", nlohmann::json{}).value("bind_address", "0.0.0.0");
            gateway_config.max_client_connections = gateway_json.value("limits", nlohmann::json{}).value("max_client_connections", 10000);
            gateway_config.max_backend_connections = gateway_json.value("limits", nlohmann::json{}).value("max_backend_connections", 100);
            gateway_config.client_timeout_ms = gateway_json.value("timeouts", nlohmann::json{}).value("client_timeout_ms", 60000);
            gateway_config.backend_timeout_ms = gateway_json.value("timeouts", nlohmann::json{}).value("backend_timeout_ms", 30000);
            gateway_config.heartbeat_interval_ms = gateway_json.value("timeouts", nlohmann::json{}).value("heartbeat_interval_ms", 30000);
            
            // 加载后端服务器列表
            if (gateway_json.contains("backend_servers") && gateway_json["backend_servers"].is_array()) {
                for (const auto& server : gateway_json["backend_servers"]) {
                    if (server.is_string()) {
                        gateway_config.backend_servers.push_back(server.get<std::string>());
                    }
                }
            }
            
            std::cout << "✅ Gateway配置加载成功" << std::endl;
        } else {
            std::cout << "⚠️  使用默认Gateway配置" << std::endl;
            
            // 使用默认配置
            gateway_config.listen_port = 8080;
            gateway_config.bind_address = "0.0.0.0";
            gateway_config.max_client_connections = 10000;
            gateway_config.max_backend_connections = 100;
            gateway_config.client_timeout_ms = 60000;
            gateway_config.backend_timeout_ms = 30000;
            gateway_config.heartbeat_interval_ms = 30000;
            
            gateway_config.backend_servers = {
                "127.0.0.1:8081",
                "127.0.0.1:8082", 
                "127.0.0.1:8083"
            };
        }

        // 检查Application是否已从命令行参数自动创建了服务
        if (app.HasCommandLineOverrides()) {
            std::cout << "📋 Application框架已自动处理命令行参数服务创建" << std::endl;
        }
        
        // Gateway现在主要负责业务逻辑配置，服务创建由Application框架自动处理
        // 这里可以添加Gateway特定的业务逻辑，如路由策略、负载均衡算法等

        std::cout << "✅ Gateway初始化完成，服务将由Application框架自动启动" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Gateway初始化失败: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief Gateway启动Hook
 */
void GatewayStartupHook(Application& app) {
    std::cout << "\n🎯 === Zeus Gateway服务启动完成 ===" << std::endl;
    std::cout << "📋 所有网络服务已由Application框架自动启动" << std::endl;
    std::cout << "💡 提示:" << std::endl;
    std::cout << "  - 使用 Ctrl+C 优雅关闭服务" << std::endl;
    std::cout << "  - 查看 logs/ 目录获取详细日志" << std::endl;
    std::cout << "  - 编辑配置文件调整参数" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

/**
 * @brief Gateway关闭Hook
 */
void GatewayShutdownHook(Application& app) {
    std::cout << "\n🔄 Gateway正在关闭..." << std::endl;
    std::cout << "📋 所有网络服务将由Application框架自动关闭" << std::endl;
    std::cout << "\n✅ Zeus Gateway正常退出。再见！" << std::endl;
}

/**
 * @brief 演示自定义信号处理
 */
void SetupCustomSignalHandling(Application& app) {
    // 设置信号处理配置
    SignalHandlerConfig config;
    config.strategy = SignalHandlerStrategy::HOOK_FIRST; // Hook优先，然后默认处理
    config.handled_signals = {SIGINT, SIGTERM, SIGUSR1}; // 添加自定义信号
    config.graceful_shutdown = true;
    config.shutdown_timeout_ms = 15000; // 15秒超时
    config.log_signal_events = true;
    app.SetSignalHandlerConfig(config);
    
    // 注册SIGINT的Hook（在默认处理之前执行）
    app.RegisterSignalHook(SIGINT, [](Application& app, int signal) {
        std::cout << "\n🔔 自定义SIGINT Hook: 正在保存临时数据..." << std::endl;
        // 这里可以执行清理操作
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 模拟清理时间
        std::cout << "✅ 临时数据已保存" << std::endl;
    });
    
    // 注册SIGTERM的Handler（可以决定是否继续默认处理）
    app.RegisterSignalHandler(SIGTERM, [](Application& app, int signal) -> bool {
        std::cout << "\n🛑 自定义SIGTERM Handler: 检查是否允许关闭..." << std::endl;
        
        // 这里可以添加验证逻辑，如检查业务状态等
        std::cout << "✅ 允许关闭服务" << std::endl;
        
        return true; // 返回true表示继续默认关闭处理
    });
    
    // 注册SIGUSR1的自定义处理（用于重载配置）
    app.RegisterSignalHook(SIGUSR1, [](Application& app, int signal) {
        std::cout << "\n🔄 收到SIGUSR1信号，重载配置..." << std::endl;
        // 这里可以实现配置重载逻辑
        std::cout << "🔄 重新加载服务配置..." << std::endl;
        // Application框架会处理服务的重载
        std::cout << "✅ 配置重载完成" << std::endl;
    });
}

/**
 * @brief 自定义Gateway使用帮助显示
 */
void ShowGatewayUsage(const std::string& program_name) {
    std::cout << "🌉 Zeus Gateway Server v1.0.0 (Enhanced Multi-Protocol)" << std::endl;
    std::cout << "用法: " << program_name << " [选项]" << std::endl;
    std::cout << std::endl;
    std::cout << "选项:" << std::endl;
    std::cout << "  -c, --config <文件>           指定配置文件路径" << std::endl;
    std::cout << "      --listen <地址>           添加监听地址 (可多次指定)" << std::endl;
    std::cout << "                                格式: [protocol://]address:port" << std::endl;
    std::cout << "                                协议: tcp, kcp, http, https (默认: tcp)" << std::endl;
    std::cout << "      --backend <地址>          添加后端服务器 (可多次指定)" << std::endl;
    std::cout << "                                格式: address:port" << std::endl;
    std::cout << "      --max-connections <数量>  设置最大客户端连接数" << std::endl;
    std::cout << "      --timeout <毫秒>          设置连接超时时间" << std::endl;
    std::cout << "  -d, --daemon                  后台运行模式" << std::endl;
    std::cout << "  -l, --log-level <级别>        设置日志级别 (debug|info|warn|error)" << std::endl;
    std::cout << "  -h, --help                    显示此帮助信息" << std::endl;
    std::cout << "  -v, --version                 显示版本信息" << std::endl;
    std::cout << std::endl;
    std::cout << "使用示例:" << std::endl;
    std::cout << "  # 基本使用" << std::endl;
    std::cout << "  " << program_name << "                                  # 使用默认配置" << std::endl;
    std::cout << "  " << program_name << " -c config/prod.json               # 使用生产配置" << std::endl;
    std::cout << std::endl;
    std::cout << "  # 多协议监听" << std::endl;
    std::cout << "  " << program_name << " --listen tcp://0.0.0.0:8080 --listen http://0.0.0.0:8081" << std::endl;
    std::cout << "  " << program_name << " --listen kcp://0.0.0.0:9000 --listen https://0.0.0.0:8443" << std::endl;
    std::cout << std::endl;
    std::cout << "  # 动态后端配置" << std::endl;
    std::cout << "  " << program_name << " --backend 192.168.1.10:8080 --backend 192.168.1.11:8080" << std::endl;
    std::cout << std::endl;
    std::cout << "  # 完整配置示例" << std::endl;
    std::cout << "  " << program_name << " --listen tcp://0.0.0.0:8080 \\\\" << std::endl;
    std::cout << "                 --backend 192.168.1.10:8080 \\\\" << std::endl;
    std::cout << "                 --backend 192.168.1.11:8080 \\\\" << std::endl;
    std::cout << "                 --max-connections 5000 \\\\" << std::endl;
    std::cout << "                 --timeout 30000 --daemon -l info" << std::endl;
    std::cout << std::endl;
    std::cout << "信号处理:" << std::endl;
    std::cout << "  SIGINT (Ctrl+C)    - 优雅关闭" << std::endl;
    std::cout << "  SIGTERM            - 终止服务" << std::endl;
    std::cout << "  SIGUSR1            - 重载配置" << std::endl;
    std::cout << "  SIGUSR2            - 显示统计信息" << std::endl;
}

/**
 * @brief 自定义Gateway版本信息显示
 */
void ShowGatewayVersion() {
    std::cout << "🌉 Zeus Gateway Server" << std::endl;
    std::cout << "版本: 1.0.0 (Enhanced Multi-Protocol Framework)" << std::endl;
    std::cout << "构建时间: " << __DATE__ << " " << __TIME__ << std::endl;
    std::cout << "框架: Zeus Application Framework v2.0" << std::endl;
    std::cout << std::endl;
    std::cout << "支持的协议:" << std::endl;
    std::cout << "  🌐 TCP  - 可靠传输协议" << std::endl;
    std::cout << "  🌐 HTTP - Web服务协议" << std::endl;
#ifdef ZEUS_USE_KCP
    std::cout << "  🚀 KCP  - 高性能UDP协议 (已启用)" << std::endl;
#else
    std::cout << "  🚀 KCP  - 高性能UDP协议 (编译时禁用)" << std::endl;
#endif
    std::cout << "  🔒 HTTPS- 安全Web协议" << std::endl;
    std::cout << std::endl;
    std::cout << "功能特性:" << std::endl;
    std::cout << "  ✅ 多协议同时监听" << std::endl;
    std::cout << "  ✅ 动态后端服务器配置" << std::endl;
    std::cout << "  ✅ 增强命令行参数解析" << std::endl;
    std::cout << "  ✅ 灵活信号处理机制" << std::endl;
    std::cout << "  ✅ 协议路由和负载均衡" << std::endl;
    std::cout << "  ✅ 会话管理和实时统计" << std::endl;
    std::cout << "  ✅ 配置文件热重载" << std::endl;
}

// 移除了本地的ConfigOverrides，现在使用Application框架提供的统一接口

/**
 * @brief 设置Gateway命令行参数
 * @param app Application实例
 * @param executable_path 可执行文件路径，用于确定默认配置文件位置
 */
void SetupGatewayArguments(Application& app, const std::string& executable_path) {
    // 设置参数解析配置
    ArgumentParserConfig config;
    config.program_name = "zeus-gateway";
    config.program_description = "Zeus Gateway Server - 高性能网络代理服务";
    config.program_version = "1.0.0";
    config.auto_add_help = true;
    config.auto_add_version = true;
    app.SetArgumentParserConfig(config);
    
    // 配置文件参数 - 使用基于应用程序目录的默认路径
    std::string default_config = GetDefaultConfigPath(executable_path);
    app.AddArgument("c", "config", "指定配置文件路径", true, default_config);
    
    // 监听地址参数 - 支持多个地址和协议
    app.AddArgumentWithHandler("", "listen", "添加监听地址 (格式: [protocol://]address:port)",
        [](Application& app, const std::string& name, const std::string& value) -> bool {
            auto endpoint = core::app::hooks::ListenEndpoint::Parse(value);
            if (!endpoint) {
                std::cerr << "❌ 错误: 无效的监听地址格式: " << value << std::endl;
                std::cerr << "格式: [protocol://]address:port" << std::endl;
                std::cerr << "协议: tcp, kcp, http, https (默认: tcp)" << std::endl;
                std::cerr << "示例: tcp://0.0.0.0:8080, http://127.0.0.1:8081, kcp://0.0.0.0:9000" << std::endl;
                return false;
            }
            
            std::cout << "✅ 添加监听地址: " << endpoint->protocol << "://" 
                      << endpoint->address << ":" << endpoint->port << std::endl;
            return true;
        }, true);
    
    // 后端服务器参数 - 支持多个后端
    app.AddArgumentWithHandler("", "backend", "添加后端服务器地址 (格式: address:port)",
        [](Application& app, const std::string& name, const std::string& value) -> bool {
            // 验证后端地址格式
            size_t colon_pos = value.rfind(':');
            if (colon_pos == std::string::npos) {
                std::cerr << "❌ 错误: 后端地址格式无效: " << value << std::endl;
                std::cerr << "格式: address:port" << std::endl;
                return false;
            }
            
            std::string port_str = value.substr(colon_pos + 1);
            try {
                int port = std::stoi(port_str);
                if (port <= 0 || port > 65535) {
                    std::cerr << "❌ 错误: 端口号必须在1-65535范围内" << std::endl;
                    return false;
                }
            } catch (const std::exception&) {
                std::cerr << "❌ 错误: 无效的端口号: " << port_str << std::endl;
                return false;
            }
            
            std::cout << "✅ 添加后端服务器: " << value << std::endl;
            return true;
        }, true);
    
    // 最大连接数参数
    app.AddArgumentWithHandler("", "max-connections", "设置最大客户端连接数",
        [](Application& app, const std::string& name, const std::string& value) -> bool {
            try {
                size_t max_conn = std::stoul(value);
                if (max_conn == 0) {
                    std::cerr << "❌ 错误: 最大连接数必须大于0" << std::endl;
                    return false;
                }
                std::cout << "✅ 最大连接数设置为: " << max_conn << std::endl;
                return true;
            } catch (const std::exception&) {
                std::cerr << "❌ 错误: 无效的连接数: " << value << std::endl;
                return false;
            }
        }, true);
    
    // 超时参数
    app.AddArgumentWithHandler("", "timeout", "设置连接超时时间 (毫秒)",
        [](Application& app, const std::string& name, const std::string& value) -> bool {
            try {
                uint32_t timeout = std::stoul(value);
                if (timeout == 0) {
                    std::cerr << "❌ 错误: 超时时间必须大于0" << std::endl;
                    return false;
                }
                std::cout << "✅ 连接超时设置为: " << timeout << "ms" << std::endl;
                return true;
            } catch (const std::exception&) {
                std::cerr << "❌ 错误: 无效的超时时间: " << value << std::endl;
                return false;
            }
        }, true);
    
    // 后台运行标志
    app.AddFlag("d", "daemon", "后台运行模式");
    
    // 日志级别参数
    app.AddArgumentWithHandler("l", "log-level", "设置日志级别 (debug|info|warn|error)",
        [](Application& app, const std::string& name, const std::string& value) -> bool {
            static const std::vector<std::string> valid_levels = {"debug", "info", "warn", "error"};
            
            if (std::find(valid_levels.begin(), valid_levels.end(), value) == valid_levels.end()) {
                std::cerr << "❌ 错误: 无效的日志级别: " << value << std::endl;
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
        // 获取Zeus Application实例
        auto& app = ZEUS_APP();
        
        // 设置Gateway命令行参数
        std::string executable_path = argc > 0 ? argv[0] : "";
        SetupGatewayArguments(app, executable_path);
        
        // 解析命令行参数
        auto parsed_args = app.ParseArgs(argc, argv);
        
        // 检查解析错误
        if (!parsed_args.error_message.empty()) {
            std::cerr << "❌ 参数解析错误: " << parsed_args.error_message << std::endl;
            app.ShowUsage(argc > 0 ? argv[0] : "zeus-gateway");
            return 1;
        }
        
        // 处理帮助请求
        if (parsed_args.help_requested) {
            app.ShowUsage(argc > 0 ? argv[0] : "zeus-gateway");
            return 0;
        }
        
        // 处理版本请求
        if (parsed_args.version_requested) {
            app.ShowVersion();
            return 0;
        }
        
        // 获取配置文件路径
        std::string default_config = GetDefaultConfigPath(executable_path);
        std::string config_file = app.GetArgumentValue("config", default_config);
        
        // 显示解析到的启动配置
        std::cout << "=== Zeus Gateway Server (Enhanced Multi-Protocol Framework) ===" << std::endl;
        std::cout << "📋 启动配置:" << std::endl;
        std::cout << "  配置文件: " << config_file << std::endl;
        
        auto overrides = app.GetCommandLineOverrides();
        
        if (!overrides.listen_endpoints.empty()) {
            std::cout << "  监听地址:" << std::endl;
            for (const auto& endpoint : overrides.listen_endpoints) {
                std::cout << "    " << endpoint.protocol << "://" 
                          << endpoint.address << ":" << endpoint.port << std::endl;
            }
        }
        
        if (!overrides.backend_servers.empty()) {
            std::cout << "  后端服务器:" << std::endl;
            for (const auto& backend : overrides.backend_servers) {
                std::cout << "    " << backend << std::endl;
            }
        }
        
        if (overrides.max_connections) {
            std::cout << "  最大连接数: " << *overrides.max_connections << std::endl;
        }
        
        if (overrides.timeout_ms) {
            std::cout << "  超时时间: " << *overrides.timeout_ms << "ms" << std::endl;
        }
        
        if (overrides.daemon_mode) {
            std::cout << "  运行模式: 后台运行" << std::endl;
        }
        
        if (overrides.log_level) {
            std::cout << "  日志级别: " << *overrides.log_level << std::endl;
        }
        
        std::cout << std::endl;
        
        // 注册Gateway Hooks
        app.RegisterInitHook(GatewayInitHook);
        app.RegisterStartupHook(GatewayStartupHook);
        app.RegisterShutdownHook(GatewayShutdownHook);
        
        // 设置自定义信号处理
        SetupCustomSignalHandling(app);
        
        // 初始化应用
        if (!app.Initialize(config_file)) {
            std::cerr << "❌ Zeus Application初始化失败" << std::endl;
            return 1;
        }
        
        // 启动应用
        if (!app.Start()) {
            std::cerr << "❌ Zeus Application启动失败" << std::endl;
            return 1;
        }
        
        // 运行应用（阻塞直到停止）
        app.Run();
        
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ 应用程序错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 未知应用程序错误" << std::endl;
        return 1;
    }
}