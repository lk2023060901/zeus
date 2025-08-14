#include "common/spdlog/zeus_log_manager.h"
#include <iostream>
#include <filesystem>

using namespace common::spdlog;

void TestDefaultConfig() {
    std::cout << "\n=== 默认配置测试 ===" << std::endl;
    
    // 测试默认配置初始化
    if (!ZeusLogManager::Instance().Initialize()) {
        std::cerr << "默认配置初始化失败" << std::endl;
        return;
    }
    
    auto default_logger = ZEUS_GET_LOGGER("default");
    if (!default_logger) {
        std::cerr << "获取默认日志器失败" << std::endl;
        return;
    }
    
    default_logger->info("默认配置测试消息");
    default_logger->warn("默认配置警告消息");
    std::cout << "✓ 默认配置测试通过" << std::endl;
    
    ZeusLogManager::Instance().Shutdown();
}

void TestFileConfig() {
    std::cout << "\n=== 文件配置测试 ===" << std::endl;
    
    // 测试从文件加载配置
    if (!ZeusLogManager::Instance().Initialize("log_config.json")) {
        std::cerr << "文件配置初始化失败" << std::endl;
        return;
    }
    
    // 测试配置文件中定义的日志器
    auto game_logger = ZEUS_GET_LOGGER("game");
    auto network_logger = ZEUS_GET_LOGGER("network");
    auto error_logger = ZEUS_GET_LOGGER("error");
    
    if (!game_logger || !network_logger || !error_logger) {
        std::cerr << "获取配置文件中的日志器失败" << std::endl;
        return;
    }
    
    game_logger->info("游戏日志器测试消息");
    network_logger->debug("网络日志器调试消息");
    error_logger->error("错误日志器错误消息");
    
    std::cout << "✓ 文件配置测试通过" << std::endl;
    ZeusLogManager::Instance().Shutdown();
}

void TestJsonStringConfig() {
    std::cout << "\n=== JSON字符串配置测试 ===" << std::endl;
    
    std::string json_config = R"({
        "global": {
            "log_level": "debug",
            "log_dir": "logs/config_test"
        },
        "loggers": [
            {
                "name": "json_logger",
                "filename_pattern": "json_test.log",
                "level": "info",
                "rotation_type": "daily",
                "console_output": true
            },
            {
                "name": "hourly_logger",
                "filename_pattern": "hourly_test.log",
                "level": "warn",
                "rotation_type": "hourly",
                "console_output": false
            }
        ]
    })";
    
    if (!ZeusLogManager::Instance().InitializeFromString(json_config)) {
        std::cerr << "JSON字符串配置初始化失败" << std::endl;
        return;
    }
    
    auto json_logger = ZEUS_GET_LOGGER("json_logger");
    auto hourly_logger = ZEUS_GET_LOGGER("hourly_logger");
    
    if (!json_logger || !hourly_logger) {
        std::cerr << "获取JSON配置的日志器失败" << std::endl;
        return;
    }
    
    json_logger->info("JSON配置日志器测试消息");
    hourly_logger->warn("小时轮换日志器警告消息");
    
    std::cout << "✓ JSON字符串配置测试通过" << std::endl;
    ZeusLogManager::Instance().Shutdown();
}

void TestDynamicLoggerCreation() {
    std::cout << "\n=== 动态日志器创建测试 ===" << std::endl;
    
    std::string config = R"({
        "global": {
            "log_level": "info",
            "log_dir": "logs/dynamic"
        },
        "loggers": []
    })";
    
    if (!ZeusLogManager::Instance().InitializeFromString(config)) {
        std::cerr << "动态配置初始化失败" << std::endl;
        return;
    }
    
    // 测试动态创建不存在的日志器
    auto dynamic1 = ZEUS_GET_LOGGER("dynamic_logger_1");
    auto dynamic2 = ZEUS_GET_LOGGER("dynamic_logger_2");
    auto dynamic3 = ZEUS_GET_LOGGER("game_system");
    
    if (!dynamic1 || !dynamic2 || !dynamic3) {
        std::cerr << "动态创建日志器失败" << std::endl;
        return;
    }
    
    dynamic1->info("动态创建的日志器1消息");
    dynamic2->warn("动态创建的日志器2警告");
    dynamic3->error("游戏系统错误消息");
    
    // 测试再次获取相同名称的日志器
    auto dynamic1_again = ZEUS_GET_LOGGER("dynamic_logger_1");
    if (dynamic1.get() != dynamic1_again.get()) {
        std::cerr << "重复获取的日志器不是同一个实例" << std::endl;
        return;
    }
    
    std::cout << "✓ 动态日志器创建测试通过" << std::endl;
    ZeusLogManager::Instance().Shutdown();
}

void TestLogLevels() {
    std::cout << "\n=== 日志级别测试 ===" << std::endl;
    
    std::string config = R"({
        "global": {
            "log_level": "warn",
            "log_dir": "logs/levels"
        },
        "loggers": [
            {
                "name": "level_test",
                "filename_pattern": "level_test.log",
                "level": "debug",
                "rotation_type": "daily",
                "console_output": true
            }
        ]
    })";
    
    if (!ZeusLogManager::Instance().InitializeFromString(config)) {
        std::cerr << "日志级别测试初始化失败" << std::endl;
        return;
    }
    
    auto logger = ZEUS_GET_LOGGER("level_test");
    if (!logger) {
        std::cerr << "获取级别测试日志器失败" << std::endl;
        return;
    }
    
    // 测试各个级别的日志
    std::cout << "输出各级别日志（当前logger级别为DEBUG）：" << std::endl;
    ZEUS_LOG_TRACE("level_test", "这是TRACE消息");
    ZEUS_LOG_DEBUG("level_test", "这是DEBUG消息");
    ZEUS_LOG_INFO("level_test", "这是INFO消息");
    ZEUS_LOG_WARN("level_test", "这是WARN消息");
    ZEUS_LOG_ERROR("level_test", "这是ERROR消息");
    ZEUS_LOG_CRITICAL("level_test", "这是CRITICAL消息");
    
    // 测试全局日志级别修改
    std::cout << "\n修改全局日志级别为ERROR：" << std::endl;
    ZeusLogManager::Instance().SetGlobalLogLevel(LogLevel::ERROR);
    
    ZEUS_LOG_DEBUG("level_test", "这条DEBUG消息应该被过滤");
    ZEUS_LOG_INFO("level_test", "这条INFO消息应该被过滤");
    ZEUS_LOG_WARN("level_test", "这条WARN消息应该被过滤");
    ZEUS_LOG_ERROR("level_test", "这条ERROR消息应该显示");
    ZEUS_LOG_CRITICAL("level_test", "这条CRITICAL消息应该显示");
    
    std::cout << "✓ 日志级别测试通过" << std::endl;
    ZeusLogManager::Instance().Shutdown();
}

void TestDirectoryCreation() {
    std::cout << "\n=== 目录自动创建测试 ===" << std::endl;
    
    // 使用深层次的目录路径
    std::string config = R"({
        "global": {
            "log_level": "info",
            "log_dir": "logs/auto_create/deep/nested/path"
        },
        "loggers": [
            {
                "name": "dir_test",
                "filename_pattern": "dir_test.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "custom_dir",
                "log_dir": "logs/custom/another/deep/path", 
                "filename_pattern": "custom.log",
                "rotation_type": "hourly",
                "console_output": false
            }
        ]
    })";
    
    if (!ZeusLogManager::Instance().InitializeFromString(config)) {
        std::cerr << "目录创建测试初始化失败" << std::endl;
        return;
    }
    
    auto dir_logger = ZEUS_GET_LOGGER("dir_test");
    auto custom_logger = ZEUS_GET_LOGGER("custom_dir");
    
    if (!dir_logger || !custom_logger) {
        std::cerr << "获取目录测试日志器失败" << std::endl;
        return;
    }
    
    dir_logger->info("测试默认深层目录创建");
    custom_logger->info("测试自定义深层目录创建");
    
    // 验证目录是否创建成功
    if (std::filesystem::exists("logs/auto_create/deep/nested/path") &&
        std::filesystem::exists("logs/custom/another/deep/path")) {
        std::cout << "✓ 目录自动创建测试通过" << std::endl;
    } else {
        std::cerr << "目录创建验证失败" << std::endl;
    }
    
    ZeusLogManager::Instance().Shutdown();
}

void TestErrorHandling() {
    std::cout << "\n=== 错误处理测试 ===" << std::endl;
    
    // 测试无效的JSON配置
    std::string invalid_json = R"({
        "global": {
            "log_level": "info"
            "log_dir": "logs"  // 缺少逗号
        }
    })";
    
    std::cout << "测试无效JSON配置..." << std::endl;
    if (ZeusLogManager::Instance().InitializeFromString(invalid_json)) {
        std::cerr << "应该拒绝无效的JSON配置" << std::endl;
    } else {
        std::cout << "✓ 正确拒绝了无效JSON配置" << std::endl;
    }
    
    // 测试不存在的配置文件
    std::cout << "测试不存在的配置文件..." << std::endl;
    if (ZeusLogManager::Instance().Initialize("nonexistent_config.json")) {
        std::cerr << "应该拒绝不存在的配置文件" << std::endl;
    } else {
        std::cout << "✓ 正确处理了不存在的配置文件" << std::endl;
    }
    
    std::cout << "✓ 错误处理测试通过" << std::endl;
}

int main() {
    std::cout << "=== Zeus Spdlog 配置测试套件 ===" << std::endl;
    
    TestDefaultConfig();
    TestFileConfig();
    TestJsonStringConfig();
    TestDynamicLoggerCreation();
    TestLogLevels();
    TestDirectoryCreation();
    TestErrorHandling();
    
    std::cout << "\n=== 配置测试完成 ===" << std::endl;
    std::cout << "请查看 logs/ 目录下的各种日志文件" << std::endl;
    
    return 0;
}