#include "common/spdlog/zeus_log_manager.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    using namespace common::spdlog;
    
    std::cout << "=== Zeus Log Library Test ===" << std::endl;
    
    // 测试1：使用配置文件初始化
    std::cout << "1. Testing initialization from config file..." << std::endl;
    if (!ZeusLogManager::Instance().Initialize("log_config.json")) {
        std::cerr << "Failed to initialize from config file" << std::endl;
        return 1;
    }
    std::cout << "   ✓ Initialization successful" << std::endl;
    
    // 测试2：获取和使用配置的日志器
    std::cout << "2. Testing configured loggers..." << std::endl;
    
    auto game_logger = ZEUS_GET_LOGGER("game");
    auto network_logger = ZEUS_GET_LOGGER("network");
    auto error_logger = ZEUS_GET_LOGGER("error");
    auto perf_logger = ZEUS_GET_LOGGER("performance");
    
    if (!game_logger || !network_logger || !error_logger || !perf_logger) {
        std::cerr << "Failed to get configured loggers" << std::endl;
        return 1;
    }
    std::cout << "   ✓ All configured loggers retrieved successfully" << std::endl;
    
    // 测试3：日志输出测试
    std::cout << "3. Testing log output..." << std::endl;
    
    // 使用便捷宏进行日志记录
    ZEUS_LOG_INFO("game", "Game started successfully");
    ZEUS_LOG_WARN("game", "Low memory warning: {}MB available", 512);
    ZEUS_LOG_ERROR("game", "Failed to load texture: {}", "player.png");
    
    ZEUS_LOG_DEBUG("network", "Connection established to server: {}", "192.168.1.100");
    ZEUS_LOG_INFO("network", "Received packet: size={}, type={}", 1024, "PLAYER_MOVE");
    
    ZEUS_LOG_ERROR("error", "Critical system error occurred");
    ZEUS_LOG_CRITICAL("error", "Application will terminate");
    
    ZEUS_LOG_INFO("performance", "FPS: {}, Memory: {}MB", 60.5, 1024);
    ZEUS_LOG_TRACE("performance", "Frame render time: {}ms", 16.67);
    
    std::cout << "   ✓ Log messages sent to all loggers" << std::endl;
    
    // 测试4：动态创建日志器
    std::cout << "4. Testing dynamic logger creation..." << std::endl;
    auto dynamic_logger = ZEUS_GET_LOGGER("dynamic_test");
    if (!dynamic_logger) {
        std::cerr << "Failed to create dynamic logger" << std::endl;
        return 1;
    }
    
    dynamic_logger->info("This is a dynamically created logger");
    dynamic_logger->warn("Dynamic logger warning message");
    std::cout << "   ✓ Dynamic logger created and used successfully" << std::endl;
    
    // 测试5：时间分割测试（快速生成多个日志）
    std::cout << "5. Testing time-based rotation..." << std::endl;
    
    for (int i = 0; i < 100; ++i) {
        ZEUS_LOG_INFO("game", "Batch log message #{}: Processing game tick", i);
        ZEUS_LOG_DEBUG("network", "Network packet #{}: data={}", i, "test_data");
        
        // 短暂延迟以模拟真实应用
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "   ✓ Generated 200 log messages for rotation testing" << std::endl;
    
    // 测试6：全局日志级别修改
    std::cout << "6. Testing global log level change..." << std::endl;
    ZeusLogManager::Instance().SetGlobalLogLevel(LogLevel::ERROR);
    
    ZEUS_LOG_INFO("game", "This INFO message should not appear");
    ZEUS_LOG_WARN("game", "This WARN message should not appear"); 
    ZEUS_LOG_ERROR("game", "This ERROR message should appear");
    ZEUS_LOG_CRITICAL("game", "This CRITICAL message should appear");
    
    std::cout << "   ✓ Global log level changed to ERROR" << std::endl;
    
    // 测试7：使用字符串配置初始化
    std::cout << "7. Testing initialization from JSON string..." << std::endl;
    
    // 重置管理器以测试字符串初始化
    ZeusLogManager::Instance().Shutdown();
    
    std::string json_config = R"({
        "global": {
            "log_level": "info",
            "log_dir": "logs/string_test"
        },
        "loggers": [
            {
                "name": "string_logger",
                "filename_pattern": "string_test.log",
                "rotation_type": "daily",
                "console_output": true
            }
        ]
    })";
    
    if (!ZeusLogManager::Instance().InitializeFromString(json_config)) {
        std::cerr << "Failed to initialize from JSON string" << std::endl;
        return 1;
    }
    
    auto string_logger = ZEUS_GET_LOGGER("string_logger");
    if (!string_logger) {
        std::cerr << "Failed to get string-configured logger" << std::endl;
        return 1;
    }
    
    string_logger->info("Logger created from JSON string configuration");
    std::cout << "   ✓ Initialization from JSON string successful" << std::endl;
    
    // 清理
    std::cout << "8. Shutting down..." << std::endl;
    ZeusLogManager::Instance().Shutdown();
    std::cout << "   ✓ Shutdown completed" << std::endl;
    
    std::cout << "\n=== All tests completed successfully! ===" << std::endl;
    std::cout << "Check the 'logs/' directory for generated log files." << std::endl;
    
    return 0;
}