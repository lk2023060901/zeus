#pragma once

/**
 * @file zeus_structured_log.h
 * @brief Zeus结构化日志 - 统一入口头文件
 * 
 * 这个头文件提供了Zeus结构化日志系统的完整功能入口，包括：
 * - Field类型系统：轻量级、类型安全的字段定义
 * - 结构化日志器：支持多种格式输出（JSON、Key-Value、LogFmt）
 * - 高性能API：基于fmt库的高效格式化
 * - 完全兼容：与现有Zeus日志系统无缝集成
 * 
 * 使用方式：
 * ```cpp
 * #include "common/spdlog/zeus_structured_log.h"
 * using namespace common::spdlog::structured;
 * 
 * // 初始化
 * ZEUS_LOG_MANAGER().Initialize();
 * 
 * // 使用Field方式
 * auto logger = ZEUS_GET_STRUCTURED_LOGGER("app");
 * logger->info(
 *     FIELD("user_id", 12345),
 *     FIELD("action", "login"),
 *     FIELD("success", true)
 * );
 * 
 * // 使用Key-Value方式
 * logger->info_kv("user_id", 12345, "action", "login");
 * 
 * // 使用便捷宏
 * ZEUS_STRUCT_INFO("app", FIELD("event", "startup"));
 * ZEUS_KV_INFO("app", "event", "startup");
 * ```
 */

// 核心组件
#include "structured/field.h"
#include "structured/structured_logger.h"

// 兼容性支持
#include "zeus_log_manager.h"

namespace common {
namespace spdlog {
namespace structured {

/**
 * @brief Zeus结构化日志版本信息
 */
struct ZeusStructuredLogVersion {
    static constexpr int MAJOR = 1;
    static constexpr int MINOR = 0;
    static constexpr int PATCH = 0;
    static constexpr const char* VERSION_STRING = "1.0.0";
};

/**
 * @brief 快速初始化结构化日志系统
 * @param config_file 配置文件路径（可选）
 * @param default_format 默认输出格式
 * @return 是否初始化成功
 */
inline bool InitializeStructuredLogging(const std::string& config_file = "",
                                       OutputFormat default_format = OutputFormat::JSON) {
    // 初始化底层的Zeus日志管理器
    auto& zeus_manager = ZEUS_LOG_MANAGER();
    bool success = config_file.empty() ? zeus_manager.Initialize() : zeus_manager.Initialize(config_file);
    
    if (success) {
        // 设置结构化日志的默认格式
        ZEUS_STRUCTURED_MANAGER().SetDefaultFormat(default_format);
    }
    
    return success;
}

/**
 * @brief 关闭结构化日志系统
 */
inline void ShutdownStructuredLogging() {
    ZEUS_LOG_MANAGER().Shutdown();
}

/**
 * @brief 获取结构化日志版本
 */
inline const char* GetVersion() {
    return ZeusStructuredLogVersion::VERSION_STRING;
}

/**
 * @brief 打印结构化日志系统信息
 */
inline void PrintStructuredLogInfo() {
    std::cout << "Zeus Structured Logging System" << std::endl;
    std::cout << "Version: " << ZeusStructuredLogVersion::VERSION_STRING << std::endl;
    std::cout << "Based on: spdlog + fmt library" << std::endl;
    std::cout << "Features: Field-based, High-performance, Type-safe" << std::endl;
    std::cout << "Formats: JSON, Key-Value, LogFmt" << std::endl;
    std::cout << std::endl;
}

/**
 * @brief 预定义的业务日志字段
 */
namespace business_fields {
    
    /**
     * @brief 用户相关字段
     */
    inline auto user_id(int64_t id) { return FIELD("user_id", id); }
    inline auto username(std::string_view name) { return FIELD("username", name); }
    inline auto user_email(std::string_view email) { return FIELD("user_email", email); }
    
    /**
     * @brief 请求相关字段
     */
    inline auto request_id(std::string_view id) { return FIELD("request_id", id); }
    inline auto session_id(std::string_view id) { return FIELD("session_id", id); }
    inline auto correlation_id(std::string_view id) { return FIELD("correlation_id", id); }
    
    /**
     * @brief HTTP相关字段
     */
    inline auto http_method(std::string_view method) { return FIELD("http_method", method); }
    inline auto http_path(std::string_view path) { return FIELD("http_path", path); }
    inline auto http_status(int status) { return FIELD("http_status", status); }
    inline auto response_time_ms(double time) { return FIELD("response_time_ms", time); }
    inline auto request_size(size_t size) { return FIELD("request_size", size); }
    inline auto response_size(size_t size) { return FIELD("response_size", size); }
    
    /**
     * @brief 错误相关字段
     */
    inline auto error_code(std::string_view code) { return FIELD("error_code", code); }
    inline auto error_message(std::string_view message) { return FIELD("error_message", message); }
    inline auto stack_trace(std::string_view trace) { return FIELD("stack_trace", trace); }
    
    /**
     * @brief 性能相关字段
     */
    inline auto cpu_usage(double usage) { return FIELD("cpu_usage", usage); }
    inline auto memory_usage_mb(double usage) { return FIELD("memory_usage_mb", usage); }
    inline auto processing_time_ms(double time) { return FIELD("processing_time_ms", time); }
    inline auto active_connections(int count) { return FIELD("active_connections", count); }
    
    /**
     * @brief 业务相关字段
     */
    inline auto event_type(std::string_view type) { return FIELD("event_type", type); }
    inline auto operation(std::string_view op) { return FIELD("operation", op); }
    inline auto resource_id(std::string_view id) { return FIELD("resource_id", id); }
    inline auto ip_address(std::string_view ip) { return FIELD("ip_address", ip); }
    
} // namespace business_fields

/**
 * @brief 常见的日志模式
 */
namespace patterns {
    
    /**
     * @brief HTTP访问日志
     */
    template<typename Logger>
    void http_access(std::shared_ptr<Logger> logger, 
                    std::string_view method, std::string_view path, 
                    int status_code, double response_time_ms,
                    std::string_view user_agent = "", 
                    std::string_view ip = "") {
        logger->info(
            business_fields::event_type("http_access"),
            business_fields::http_method(method),
            business_fields::http_path(path),
            business_fields::http_status(status_code),
            business_fields::response_time_ms(response_time_ms),
            FIELD("user_agent", user_agent),
            business_fields::ip_address(ip),
            fields::timestamp()
        );
    }
    
    /**
     * @brief 错误日志
     */
    template<typename Logger>
    void error_event(std::shared_ptr<Logger> logger,
                    std::string_view error_code, std::string_view error_message,
                    std::string_view context = "", std::string_view trace = "") {
        logger->error(
            business_fields::event_type("error"),
            business_fields::error_code(error_code),
            business_fields::error_message(error_message),
            FIELD("context", context),
            business_fields::stack_trace(trace),
            fields::timestamp()
        );
    }
    
    /**
     * @brief 性能指标日志
     */
    template<typename Logger>
    void performance_metric(std::shared_ptr<Logger> logger,
                           std::string_view operation, double duration_ms,
                           double cpu_usage = 0.0, double memory_mb = 0.0) {
        logger->info(
            business_fields::event_type("performance_metric"),
            business_fields::operation(operation),
            business_fields::processing_time_ms(duration_ms),
            business_fields::cpu_usage(cpu_usage),
            business_fields::memory_usage_mb(memory_mb),
            fields::timestamp()
        );
    }
    
    /**
     * @brief 用户活动日志
     */
    template<typename Logger>
    void user_activity(std::shared_ptr<Logger> logger,
                      int64_t user_id, std::string_view action,
                      std::string_view resource = "", bool success = true) {
        logger->info(
            business_fields::event_type("user_activity"),
            business_fields::user_id(user_id),
            FIELD("action", action),
            business_fields::resource_id(resource),
            FIELD("success", success),
            fields::timestamp()
        );
    }
    
} // namespace patterns

} // namespace structured
} // namespace spdlog
} // namespace common

// ===========================================
// 全局便捷宏定义
// ===========================================

// 快速初始化
#define ZEUS_INIT_STRUCTURED_LOG(config_file) \
    common::spdlog::structured::InitializeStructuredLogging(config_file)

#define ZEUS_INIT_STRUCTURED_LOG_DEFAULT() \
    common::spdlog::structured::InitializeStructuredLogging()

// 业务字段便捷宏
#define USER_ID(id) common::spdlog::structured::business_fields::user_id(id)
#define REQUEST_ID(id) common::spdlog::structured::business_fields::request_id(id)
#define ERROR_CODE(code) common::spdlog::structured::business_fields::error_code(code)
#define HTTP_METHOD(method) common::spdlog::structured::business_fields::http_method(method)
#define RESPONSE_TIME(time) common::spdlog::structured::business_fields::response_time_ms(time)

// 常见模式便捷宏
#define ZEUS_LOG_HTTP_ACCESS(logger, method, path, status, time) \
    common::spdlog::structured::patterns::http_access(logger, method, path, status, time)

#define ZEUS_LOG_ERROR_EVENT(logger, code, message) \
    common::spdlog::structured::patterns::error_event(logger, code, message)

#define ZEUS_LOG_PERFORMANCE(logger, operation, duration) \
    common::spdlog::structured::patterns::performance_metric(logger, operation, duration)