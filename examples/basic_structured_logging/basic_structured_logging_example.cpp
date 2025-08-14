/**
 * @file basic_structured_logging_example.cpp
 * @brief 基本结构化日志使用示例
 * 
 * 这个示例演示了如何使用Zeus结构化日志系统的基本功能：
 * - Field对象方式记录日志
 * - Key-Value方式记录日志
 * - 不同数据类型的支持
 * - 预定义字段的使用
 */

#include "common/spdlog/zeus_structured_log.h"
#include <iostream>
#include <filesystem>

using namespace common::spdlog::structured;

/**
 * @brief 演示基本的Field对象使用
 */
void demo_field_objects() {
    std::cout << "\n=== Field对象方式示例 ===" << std::endl;
    
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("basic");
    if (!logger) {
        std::cerr << "Failed to get structured logger" << std::endl;
        return;
    }
    
    // 1. 基本数据类型
    logger->info(
        FIELD("string_value", "Hello World"),
        FIELD("integer_value", 42),
        FIELD("float_value", 3.14159),
        FIELD("boolean_value", true)
    );
    
    // 2. 不同整数类型
    logger->debug(
        FIELD("int8", int8_t(127)),
        FIELD("int16", int16_t(32767)),
        FIELD("int32", int32_t(2147483647)),
        FIELD("int64", int64_t(9223372036854775807LL))
    );
    
    // 3. 字符串类型变体
    std::string std_string = "std::string";
    std::string_view string_view = "string_view";
    const char* c_string = "c_string";
    
    logger->info(
        FIELD("std_string", std_string),
        FIELD("string_view", string_view),
        FIELD("c_string", c_string)
    );
}

/**
 * @brief 演示Key-Value方式使用
 */
void demo_key_value() {
    std::cout << "\n=== Key-Value方式示例 ===" << std::endl;
    
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("basic");
    if (!logger) return;
    
    // 1. 基本key-value记录
    logger->info_kv(
        "operation", "user_login",
        "user_id", 12345,
        "success", true,
        "duration_ms", 234.56
    );
    
    // 2. 不同日志级别
    logger->trace_kv("level", "trace", "message", "This is a trace message");
    logger->debug_kv("level", "debug", "message", "This is a debug message");
    logger->warn_kv("level", "warn", "message", "This is a warning message");
    logger->error_kv("level", "error", "message", "This is an error message");
}

/**
 * @brief 演示预定义字段的使用
 */
void demo_predefined_fields() {
    std::cout << "\n=== 预定义字段示例 ===" << std::endl;
    
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("basic");
    if (!logger) return;
    
    // 1. 通用预定义字段
    logger->info(
        fields::message("Application started successfully"),
        fields::level("INFO"),
        fields::timestamp(),
        fields::thread_id()
    );
    
    // 2. 业务预定义字段
    logger->info(
        business_fields::event_type("user_action"),
        business_fields::user_id(98765),
        business_fields::operation("profile_update"),
        business_fields::ip_address("192.168.1.100")
    );
    
    // 3. HTTP相关字段
    logger->info(
        business_fields::http_method("POST"),
        business_fields::http_path("/api/users/profile"),
        business_fields::http_status(200),
        business_fields::response_time_ms(156.78),
        business_fields::request_size(1024),
        business_fields::response_size(512)
    );
}

/**
 * @brief 演示不同输出格式
 */
void demo_output_formats() {
    std::cout << "\n=== 不同输出格式示例 ===" << std::endl;
    
    // JSON格式（默认）
    std::cout << "JSON格式输出：" << std::endl;
    auto json_logger = ZEUS_GET_STRUCTURED_LOGGER("json_format");
    json_logger->set_format(OutputFormat::JSON);
    json_logger->info(
        FIELD("format", "JSON"),
        FIELD("user_id", 12345),
        FIELD("action", "demo"),
        FIELD("timestamp", std::chrono::system_clock::now())
    );
    
    // Key-Value格式
    std::cout << "\nKey-Value格式输出：" << std::endl;
    auto kv_logger = ZEUS_GET_STRUCTURED_LOGGER("kv_format");
    kv_logger->set_format(OutputFormat::KEY_VALUE);
    kv_logger->info(
        FIELD("format", "KEY_VALUE"),
        FIELD("user_id", 12345),
        FIELD("action", "demo"),
        FIELD("success", true)
    );
    
    // LogFmt格式
    std::cout << "\nLogFmt格式输出：" << std::endl;
    auto logfmt_logger = ZEUS_GET_STRUCTURED_LOGGER("logfmt_format");
    logfmt_logger->set_format(OutputFormat::LOGFMT);
    logfmt_logger->info(
        FIELD("format", "LOGFMT"),
        FIELD("user_id", 12345),
        FIELD("action", "demo"),
        FIELD("completed", true)
    );
}

/**
 * @brief 演示便捷宏的使用
 */
void demo_convenience_macros() {
    std::cout << "\n=== 便捷宏示例 ===" << std::endl;
    
    // Field方式的便捷宏
    ZEUS_STRUCT_INFO("basic", 
        FIELD("macro_type", "ZEUS_STRUCT_INFO"),
        FIELD("user_id", 11111)
    );
    
    ZEUS_STRUCT_DEBUG("basic",
        FIELD("macro_type", "ZEUS_STRUCT_DEBUG"),
        FIELD("debug_info", "This is debug information")
    );
    
    // Key-Value方式的便捷宏
    ZEUS_KV_INFO("basic", 
        "macro_type", "ZEUS_KV_INFO",
        "operation", "macro_demo"
    );
    
    ZEUS_KV_ERROR("basic",
        "macro_type", "ZEUS_KV_ERROR", 
        "error_code", "DEMO_001",
        "error_message", "This is a demo error"
    );
}

/**
 * @brief 演示常见日志模式
 */
void demo_common_patterns() {
    std::cout << "\n=== 常见日志模式示例 ===" << std::endl;
    
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("basic");
    if (!logger) return;
    
    // HTTP访问日志模式
    patterns::http_access(logger, "GET", "/api/health", 200, 12.34, "Zeus-Client/1.0", "127.0.0.1");
    
    // 错误事件日志模式
    patterns::error_event(logger, "VALIDATION_ERROR", "Invalid input parameters", "user_registration");
    
    // 性能指标日志模式
    patterns::performance_metric(logger, "database_query", 89.45, 23.5, 256.7);
    
    // 用户活动日志模式
    patterns::user_activity(logger, 54321, "password_change", "user_settings", true);
}

/**
 * @brief 主函数
 */
int main() {
    try {
        std::cout << "=== Zeus结构化日志基本使用示例 ===" << std::endl;
        std::cout << "版本: " << GetVersion() << std::endl;
        
        // 创建日志目录
        std::filesystem::create_directories("logs");
        
        // 初始化结构化日志系统
        if (!InitializeStructuredLogging("", OutputFormat::JSON)) {
            std::cerr << "Failed to initialize structured logging" << std::endl;
            return 1;
        }
        
        // 打印框架信息
        PrintStructuredLogInfo();
        
        // 运行各种演示
        demo_field_objects();
        demo_key_value();
        demo_predefined_fields();
        demo_output_formats();
        demo_convenience_macros();
        demo_common_patterns();
        
        std::cout << "\n=== 基本示例完成 ===" << std::endl;
        std::cout << "请查看 logs/ 目录中的日志文件以查看输出结果。" << std::endl;
        
        // 关闭日志系统
        ShutdownStructuredLogging();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "示例运行错误: " << e.what() << std::endl;
        return 1;
    }
}