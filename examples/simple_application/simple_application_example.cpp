/**
 * @file simple_application_example.cpp
 * @brief Zeus Application Framework - 简单应用程序示例
 * 
 * 这个示例演示了如何使用Zeus应用程序框架创建一个完整的应用：
 * - 应用程序生命周期管理
 * - 配置文件加载和管理
 * - HTTP服务创建和配置
 * - Hook系统使用
 * - 优雅的启动和关闭流程
 */

#include "core/zeus_application.h"
#include <iostream>
#include <filesystem>

using namespace core::app;

int main() {
    try {
        std::cout << "=== Zeus应用程序框架示例 ===" << std::endl;
        
        // 打印框架信息
        ZEUS_PRINT_INFO();
        
        // 获取应用程序实例
        auto& app = ZEUS_APP();
        
        // 快速设置常用Hook
        ZEUS_QUICK_SETUP();
        
        // 注册自定义启动Hook
        app.RegisterStartupHook([](Application& app) {
            std::cout << "✅ 自定义启动Hook执行完成！" << std::endl;
            const auto& config = app.GetConfig().GetApplicationConfig();
            std::cout << "应用程序: " << config.name 
                      << " v" << config.version << std::endl;
            std::cout << "Lua脚本路径: " << config.lua_script_path << std::endl;
        });
        
        // 注册HTTP服务初始化Hook
        app.RegisterInitHook([](Application& app) -> bool {
            const auto& listeners = app.GetConfig().GetListenerConfigs();
            if (listeners.empty()) {
                std::cout << "📡 未找到监听器配置，创建默认HTTP服务器..." << std::endl;
                
                // 创建HTTP服务选项
                HttpServiceOptions options;
                options.request_handler = [](const std::string& method, const std::string& path,
                                           const std::unordered_map<std::string, std::string>& headers,
                                           const std::string& body, std::string& response) {
                    std::cout << "🌐 HTTP请求: " << method << " " << path << std::endl;
                    
                    if (path == "/") {
                        response = R"({
                            "message": "欢迎使用Zeus应用程序框架！",
                            "version": "1.0.0",
                            "framework": "Zeus",
                            "features": [
                                "高性能网络通信",
                                "结构化日志系统", 
                                "依赖注入容器",
                                "配置管理",
                                "Hook扩展系统"
                            ],
                            "endpoints": {
                                "/": "欢迎页面",
                                "/health": "健康检查",
                                "/info": "应用程序信息",
                                "/metrics": "性能指标",
                                "/config": "配置信息"
                            }
                        })";
                    } else if (path == "/health") {
                        response = R"({
                            "status": "healthy", 
                            "uptime": "running",
                            "services": "all_running",
                            "timestamp": ")" + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count()) + R"("
                        })";
                    } else if (path == "/info") {
                        response = R"({
                            "application": "Zeus示例应用",
                            "framework_version": "1.0.0",
                            "build_type": ")" + std::string(ZEUS_FRAMEWORK_BUILD_TYPE()) + R"(",
                            "worker_threads": )" + std::to_string(app.GetWorkerThreadCount()) + R"(,
                            "total_services": )" + std::to_string(app.GetServiceRegistry().GetTotalServiceCount()) + R"(
                        })";
                    } else if (path == "/metrics") {
                        response = R"({
                            "performance": {
                                "active_connections": )" + std::to_string(app.GetServiceRegistry().GetRunningServiceCount()) + R"(,
                                "total_requests": 0,
                                "avg_response_time_ms": 12.34,
                                "memory_usage_mb": 64.5
                            },
                            "system": {
                                "cpu_usage": 23.4,
                                "load_average": 1.2,
                                "disk_usage_percent": 45.6
                            }
                        })";
                    } else if (path == "/config") {
                        const auto& app_config = app.GetConfig().GetApplicationConfig();
                        response = R"({
                            "application_name": ")" + app_config.name + R"(",
                            "version": ")" + app_config.version + R"(",
                            "lua_script_path": ")" + app_config.lua_script_path + R"(",
                            "worker_threads": )" + std::to_string(app.GetWorkerThreadCount()) + R"(,
                            "listeners": )" + std::to_string(app.GetConfig().GetListenerConfigs().size()) + R"(,
                            "connectors": )" + std::to_string(app.GetConfig().GetConnectorConfigs().size()) + R"(
                        })";
                    } else {
                        response = R"({
                            "error": "Not Found", 
                            "code": 404,
                            "message": "请求的路径不存在",
                            "available_endpoints": ["/", "/health", "/info", "/metrics", "/config"]
                        })";
                    }
                };
                
                // 创建默认HTTP服务器配置
                ListenerConfig config = ApplicationUtils::CreateHttpEchoServer();
                config.name = "zeus_demo_server";
                
                // 创建服务
                bool success = app.CreateHttpService(config, options);
                if (success) {
                    std::cout << "✅ HTTP服务创建成功！" << std::endl;
                } else {
                    std::cerr << "❌ HTTP服务创建失败！" << std::endl;
                }
                return success;
            }
            return true;
        });
        
        // 注册关闭Hook
        app.RegisterShutdownHook([](Application& app) {
            std::cout << "🔄 应用程序正在关闭..." << std::endl;
            std::cout << "📊 服务统计:" << std::endl;
            std::cout << "  总服务数: " << app.GetServiceRegistry().GetTotalServiceCount() << std::endl;
            std::cout << "  运行中服务: " << app.GetServiceRegistry().GetRunningServiceCount() << std::endl;
            std::cout << "  工作线程数: " << app.GetWorkerThreadCount() << std::endl;
        });
        
        // 尝试初始化应用程序
        bool initialized = false;
        
        if (std::filesystem::exists("config.json")) {
            std::cout << "📄 从config.json加载配置..." << std::endl;
            initialized = app.Initialize("config.json");
        } else {
            std::cout << "📄 config.json未找到，创建默认配置..." << std::endl;
            
            // 创建默认配置
            if (ApplicationUtils::CreateDefaultConfig("config.json", "zeus_demo_app")) {
                std::cout << "✅ 默认config.json已创建，正在加载..." << std::endl;
                initialized = app.Initialize("config.json");
            } else {
                std::cerr << "❌ 创建默认配置失败" << std::endl;
                return 1;
            }
        }
        
        if (!initialized) {
            std::cerr << "❌ 应用程序初始化失败" << std::endl;
            return 1;
        }
        
        // 显示启动信息
        std::cout << "\n🎉 === 应用程序初始化成功 ===" << std::endl;
        std::cout << "🌐 HTTP服务器地址: http://localhost:8080" << std::endl;
        std::cout << "📡 可用的API端点:" << std::endl;
        std::cout << "  http://localhost:8080/        - 欢迎页面" << std::endl;
        std::cout << "  http://localhost:8080/health  - 健康检查" << std::endl;
        std::cout << "  http://localhost:8080/info    - 应用信息" << std::endl;
        std::cout << "  http://localhost:8080/metrics - 性能指标" << std::endl;
        std::cout << "  http://localhost:8080/config  - 配置信息" << std::endl;
        std::cout << "\n💡 提示:" << std::endl;
        std::cout << "  - 使用 Ctrl+C 优雅关闭服务器" << std::endl;
        std::cout << "  - 查看 logs/ 目录获取详细日志" << std::endl;
        std::cout << "  - 编辑 config.json 自定义配置" << std::endl;
        std::cout << "  - 脚本文件请放置在 scripts/ 目录" << std::endl;
        std::cout << "\n🚀 服务器启动中..." << std::endl;
        std::cout << "========================================\n" << std::endl;
        
        // 运行应用程序（阻塞直到停止）
        app.Run();
        
        std::cout << "\n✅ 应用程序正常退出。再见！" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 应用程序错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 未知应用程序错误" << std::endl;
        return 1;
    }
}