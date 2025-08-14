#pragma once

#include "field.h"
#include "../zeus_log_manager.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <type_traits>
#include <string_view>
#include <utility>

namespace common {
namespace spdlog {
namespace structured {

/**
 * @brief 结构化日志输出格式
 */
enum class OutputFormat {
    JSON,           // {"key1":"value1","key2":"value2"}
    KEY_VALUE,      // key1=value1 key2=value2
    LOGFMT          // key1=value1, key2=value2
};

/**
 * @brief 结构化日志器类
 * 
 * 提供基于Field的高性能结构化日志功能，支持：
 * 1. Field对象方式：logger.info(Field("key", value), ...)
 * 2. Key-Value方式：logger.info("key1", value1, "key2", value2, ...)
 * 3. 混合方式：支持Field对象和key-value混用
 */
class StructuredLogger {
private:
    std::shared_ptr<::spdlog::logger> logger_;
    OutputFormat format_ = OutputFormat::JSON;
    
public:
    /**
     * @brief 构造函数
     * @param logger spdlog日志器实例
     * @param format 输出格式
     */
    explicit StructuredLogger(std::shared_ptr<::spdlog::logger> logger, 
                             OutputFormat format = OutputFormat::JSON)
        : logger_(std::move(logger)), format_(format) {}
    
    /**
     * @brief 获取底层的spdlog logger
     */
    std::shared_ptr<::spdlog::logger> get_logger() const { return logger_; }
    
    /**
     * @brief 设置输出格式
     */
    void set_format(OutputFormat format) { format_ = format; }
    
    /**
     * @brief 获取输出格式
     */
    OutputFormat get_format() const { return format_; }
    
    // ===========================================
    // Field对象方式的日志方法
    // ===========================================
    
    /**
     * @brief 使用Field对象记录trace级别日志
     */
    template<typename... Fields>
    void trace(Fields&&... fields) {
        log(::spdlog::level::trace, std::forward<Fields>(fields)...);
    }
    
    /**
     * @brief 使用Field对象记录debug级别日志
     */
    template<typename... Fields>
    void debug(Fields&&... fields) {
        log(::spdlog::level::debug, std::forward<Fields>(fields)...);
    }
    
    /**
     * @brief 使用Field对象记录info级别日志
     */
    template<typename... Fields>
    void info(Fields&&... fields) {
        log(::spdlog::level::info, std::forward<Fields>(fields)...);
    }
    
    /**
     * @brief 使用Field对象记录warn级别日志
     */
    template<typename... Fields>
    void warn(Fields&&... fields) {
        log(::spdlog::level::warn, std::forward<Fields>(fields)...);
    }
    
    /**
     * @brief 使用Field对象记录error级别日志
     */
    template<typename... Fields>
    void error(Fields&&... fields) {
        log(::spdlog::level::err, std::forward<Fields>(fields)...);
    }
    
    /**
     * @brief 使用Field对象记录critical级别日志
     */
    template<typename... Fields>
    void critical(Fields&&... fields) {
        log(::spdlog::level::critical, std::forward<Fields>(fields)...);
    }
    
    // ===========================================
    // Key-Value方式的日志方法
    // ===========================================
    
    /**
     * @brief 使用key-value方式记录trace级别日志
     */
    template<typename... Args>
    void trace_kv(Args&&... args) {
        static_assert(sizeof...(args) % 2 == 0, "Arguments must be key-value pairs");
        log_kv(::spdlog::level::trace, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 使用key-value方式记录debug级别日志
     */
    template<typename... Args>
    void debug_kv(Args&&... args) {
        static_assert(sizeof...(args) % 2 == 0, "Arguments must be key-value pairs");
        log_kv(::spdlog::level::debug, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 使用key-value方式记录info级别日志
     */
    template<typename... Args>
    void info_kv(Args&&... args) {
        static_assert(sizeof...(args) % 2 == 0, "Arguments must be key-value pairs");
        log_kv(::spdlog::level::info, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 使用key-value方式记录warn级别日志
     */
    template<typename... Args>
    void warn_kv(Args&&... args) {
        static_assert(sizeof...(args) % 2 == 0, "Arguments must be key-value pairs");
        log_kv(::spdlog::level::warn, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 使用key-value方式记录error级别日志
     */
    template<typename... Args>
    void error_kv(Args&&... args) {
        static_assert(sizeof...(args) % 2 == 0, "Arguments must be key-value pairs");
        log_kv(::spdlog::level::err, std::forward<Args>(args)...);
    }
    
    /**
     * @brief 使用key-value方式记录critical级别日志
     */
    template<typename... Args>
    void critical_kv(Args&&... args) {
        static_assert(sizeof...(args) % 2 == 0, "Arguments must be key-value pairs");
        log_kv(::spdlog::level::critical, std::forward<Args>(args)...);
    }

private:
    /**
     * @brief 通用的Field对象日志记录方法
     */
    template<typename... Fields>
    void log(::spdlog::level::level_enum level, Fields&&... fields) {
        if (!logger_ || !logger_->should_log(level)) {
            return;
        }
        
        // 创建字段容器
        auto container = make_fields(std::forward<Fields>(fields)...);
        
        // 根据格式类型进行格式化
        switch (format_) {
            case OutputFormat::JSON:
                logger_->log(level, "{}", container);
                break;
            case OutputFormat::KEY_VALUE:
                logger_->log(level, "{}", format_as_key_value(container));
                break;
            case OutputFormat::LOGFMT:
                logger_->log(level, "{}", format_as_logfmt(container));
                break;
        }
    }
    
    /**
     * @brief 通用的key-value日志记录方法
     */
    template<typename... Args>
    void log_kv(::spdlog::level::level_enum level, Args&&... args) {
        if (!logger_ || !logger_->should_log(level)) {
            return;
        }
        
        // 将key-value参数转换为Field对象
        auto fields = make_fields_from_kv(std::forward<Args>(args)...);
        
        // 根据格式类型进行格式化
        switch (format_) {
            case OutputFormat::JSON:
                logger_->log(level, "{}", fields);
                break;
            case OutputFormat::KEY_VALUE:
                logger_->log(level, "{}", format_as_key_value(fields));
                break;
            case OutputFormat::LOGFMT:
                logger_->log(level, "{}", format_as_logfmt(fields));
                break;
        }
    }
    
    /**
     * @brief 将key-value参数对转换为Field对象容器
     */
    template<typename K, typename V, typename... Rest>
    auto make_fields_from_kv(K&& key, V&& value, Rest&&... rest) {
        if constexpr (sizeof...(rest) == 0) {
            // 最后一对参数
            return make_fields(make_field(std::forward<K>(key), std::forward<V>(value)));
        } else {
            // 递归处理剩余参数
            auto current_field = make_field(std::forward<K>(key), std::forward<V>(value));
            auto rest_fields = make_fields_from_kv(std::forward<Rest>(rest)...);
            
            // 合并字段容器
            return merge_fields(make_fields(current_field), rest_fields);
        }
    }
    
    /**
     * @brief 合并两个字段容器
     */
    template<typename Container1, typename Container2>
    auto merge_fields(Container1&& c1, Container2&& c2) {
        return std::apply([&c2](auto&&... fields1) {
            return std::apply([&fields1...](auto&&... fields2) {
                return make_fields(std::forward<decltype(fields1)>(fields1)..., 
                                 std::forward<decltype(fields2)>(fields2)...);
            }, c2.fields);
        }, c1.fields);
    }
    
    /**
     * @brief 将字段容器格式化为key=value格式
     */
    template<typename Container>
    std::string format_as_key_value(const Container& container) {
        std::string result;
        bool first = true;
        
        std::apply([&](const auto&... fields) {
            ((first ? (first = false, result += fmt::format("{}={}", fields.key(), format_value(fields))) 
                   : result += fmt::format(" {}={}", fields.key(), format_value(fields))), ...);
        }, container.fields);
        
        return result;
    }
    
    /**
     * @brief 将字段容器格式化为logfmt格式
     */
    template<typename Container>
    std::string format_as_logfmt(const Container& container) {
        std::string result;
        bool first = true;
        
        std::apply([&](const auto&... fields) {
            ((first ? (first = false, result += fmt::format("{}={}", fields.key(), format_value(fields))) 
                   : result += fmt::format(", {}={}", fields.key(), format_value(fields))), ...);
        }, container.fields);
        
        return result;
    }
    
    /**
     * @brief 格式化单个字段的值
     */
    template<typename Field>
    std::string format_value(const Field& field) {
        const auto& value = field.value();
        
        if constexpr (std::is_arithmetic_v<typename Field::value_type>) {
            if constexpr (std::is_same_v<typename Field::value_type, bool>) {
                return value ? "true" : "false";
            } else {
                return std::to_string(value);
            }
        } else {
            // 字符串类型需要引号包围（如果包含空格的话）
            std::string str_value = field.to_string();
            if (str_value.find(' ') != std::string::npos || str_value.find(',') != std::string::npos) {
                return "\"" + str_value + "\"";
            }
            return str_value;
        }
    }
};

/**
 * @brief 结构化日志管理器
 * 扩展现有的ZeusLogManager以支持结构化日志
 */
class ZeusStructuredLogManager {
private:
    ZeusLogManager& zeus_manager_;
    std::unordered_map<std::string, std::unique_ptr<StructuredLogger>> structured_loggers_;
    std::mutex mutex_;
    OutputFormat default_format_ = OutputFormat::JSON;
    
public:
    /**
     * @brief 构造函数
     */
    ZeusStructuredLogManager() : zeus_manager_(ZeusLogManager::Instance()) {}
    
    /**
     * @brief 获取单例实例
     */
    static ZeusStructuredLogManager& Instance() {
        static ZeusStructuredLogManager instance;
        return instance;
    }
    
    /**
     * @brief 获取结构化日志器
     * @param name 日志器名称
     * @param format 输出格式（可选）
     */
    std::shared_ptr<StructuredLogger> GetStructuredLogger(const std::string& name, 
                                                         OutputFormat format = OutputFormat::JSON) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = structured_loggers_.find(name);
        if (it == structured_loggers_.end()) {
            // 获取底层的spdlog logger
            auto spdlog_logger = zeus_manager_.GetLogger(name);
            if (!spdlog_logger) {
                return nullptr;
            }
            
            // 创建结构化日志器
            auto structured_logger = std::make_unique<StructuredLogger>(spdlog_logger, format);
            auto result = std::shared_ptr<StructuredLogger>(structured_logger.get(), [](StructuredLogger*){});
            
            structured_loggers_[name] = std::move(structured_logger);
            return result;
        }
        
        // 返回已存在的日志器
        return std::shared_ptr<StructuredLogger>(it->second.get(), [](StructuredLogger*){});
    }
    
    /**
     * @brief 设置默认输出格式
     */
    void SetDefaultFormat(OutputFormat format) {
        std::lock_guard<std::mutex> lock(mutex_);
        default_format_ = format;
    }
    
    /**
     * @brief 获取默认输出格式
     */
    OutputFormat GetDefaultFormat() const {
        return default_format_;
    }
};

} // namespace structured
} // namespace spdlog
} // namespace common

// ===========================================
// 便捷宏定义
// ===========================================

#define ZEUS_STRUCTURED_MANAGER() common::spdlog::structured::ZeusStructuredLogManager::Instance()
#define ZEUS_GET_STRUCTURED_LOGGER(name) ZEUS_STRUCTURED_MANAGER().GetStructuredLogger(name)

// Field方式的便捷宏
#define ZEUS_STRUCT_TRACE(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->trace(__VA_ARGS__)
#define ZEUS_STRUCT_DEBUG(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->debug(__VA_ARGS__)
#define ZEUS_STRUCT_INFO(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->info(__VA_ARGS__)
#define ZEUS_STRUCT_WARN(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->warn(__VA_ARGS__)
#define ZEUS_STRUCT_ERROR(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->error(__VA_ARGS__)
#define ZEUS_STRUCT_CRITICAL(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->critical(__VA_ARGS__)

// Key-Value方式的便捷宏
#define ZEUS_KV_TRACE(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->trace_kv(__VA_ARGS__)
#define ZEUS_KV_DEBUG(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->debug_kv(__VA_ARGS__)
#define ZEUS_KV_INFO(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->info_kv(__VA_ARGS__)
#define ZEUS_KV_WARN(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->warn_kv(__VA_ARGS__)
#define ZEUS_KV_ERROR(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->error_kv(__VA_ARGS__)
#define ZEUS_KV_CRITICAL(logger_name, ...) if(auto logger = ZEUS_GET_STRUCTURED_LOGGER(logger_name)) logger->critical_kv(__VA_ARGS__)