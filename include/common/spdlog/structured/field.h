#pragma once

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <fmt/format.h>
#include <chrono>
#include <thread>

namespace common {
namespace spdlog {
namespace structured {

/**
 * @brief Field类型枚举，用于运行时类型识别和优化
 */
enum class FieldType : uint8_t {
    BOOL,
    INT8, INT16, INT32, INT64,
    UINT8, UINT16, UINT32, UINT64,
    FLOAT, DOUBLE,
    STRING, STRING_VIEW,
    TIMESTAMP,
    CUSTOM
};

/**
 * @brief 获取类型对应的FieldType枚举值
 */
template<typename T>
constexpr FieldType get_field_type() {
    using DecayedT = std::decay_t<T>;
    
    if constexpr (std::is_same_v<DecayedT, bool>) return FieldType::BOOL;
    else if constexpr (std::is_same_v<DecayedT, int8_t>) return FieldType::INT8;
    else if constexpr (std::is_same_v<DecayedT, int16_t>) return FieldType::INT16;
    else if constexpr (std::is_same_v<DecayedT, int32_t> || std::is_same_v<DecayedT, int>) return FieldType::INT32;
    else if constexpr (std::is_same_v<DecayedT, int64_t> || std::is_same_v<DecayedT, long> || std::is_same_v<DecayedT, long long>) return FieldType::INT64;
    else if constexpr (std::is_same_v<DecayedT, uint8_t>) return FieldType::UINT8;
    else if constexpr (std::is_same_v<DecayedT, uint16_t>) return FieldType::UINT16;
    else if constexpr (std::is_same_v<DecayedT, uint32_t> || std::is_same_v<DecayedT, unsigned int>) return FieldType::UINT32;
    else if constexpr (std::is_same_v<DecayedT, uint64_t> || std::is_same_v<DecayedT, unsigned long> || std::is_same_v<DecayedT, unsigned long long>) return FieldType::UINT64;
    else if constexpr (std::is_same_v<DecayedT, float>) return FieldType::FLOAT;
    else if constexpr (std::is_same_v<DecayedT, double>) return FieldType::DOUBLE;
    else if constexpr (std::is_same_v<DecayedT, std::string>) return FieldType::STRING;
    else if constexpr (std::is_same_v<DecayedT, std::string_view>) return FieldType::STRING_VIEW;
    else if constexpr (std::is_same_v<DecayedT, const char*>) return FieldType::STRING_VIEW;
    else if constexpr (std::is_same_v<DecayedT, std::chrono::system_clock::time_point>) return FieldType::TIMESTAMP;
    else return FieldType::CUSTOM;
}

/**
 * @brief 轻量级Field类，支持高效的键值对存储
 * 
 * 设计原则：
 * 1. 零开销抽象 - 编译时确定类型，避免运行时开销
 * 2. 延迟序列化 - 只在真正需要格式化时才进行字符串转换
 * 3. 内存高效 - 优先使用栈分配，避免不必要的堆分配
 * 4. 类型安全 - 编译时类型检查，避免运行时错误
 */
template<typename T>
class Field {
public:
    using value_type = std::decay_t<T>;
    static constexpr FieldType field_type = get_field_type<T>();
    
private:
    std::string_view key_;
    value_type value_;

public:
    /**
     * @brief 构造函数
     * @param key 字段名（必须保证生命周期）
     * @param value 字段值
     */
    constexpr Field(std::string_view key, T&& value) noexcept
        : key_(key), value_(std::forward<T>(value)) {}
    
    /**
     * @brief 拷贝构造函数
     */
    Field(const Field& other) = default;
    
    /**
     * @brief 移动构造函数
     */
    Field(Field&& other) noexcept = default;
    
    /**
     * @brief 拷贝赋值操作符
     */
    Field& operator=(const Field& other) = default;
    
    /**
     * @brief 移动赋值操作符
     */
    Field& operator=(Field&& other) noexcept = default;
    
    /**
     * @brief 获取字段名
     */
    constexpr std::string_view key() const noexcept { return key_; }
    
    /**
     * @brief 获取字段值
     */
    constexpr const value_type& value() const noexcept { return value_; }
    
    /**
     * @brief 获取字段类型
     */
    constexpr FieldType type() const noexcept { return field_type; }
    
    /**
     * @brief 检查字段值是否为数值类型
     */
    constexpr bool is_numeric() const noexcept {
        return field_type >= FieldType::INT8 && field_type <= FieldType::DOUBLE;
    }
    
    /**
     * @brief 检查字段值是否为字符串类型
     */
    constexpr bool is_string() const noexcept {
        return field_type == FieldType::STRING || field_type == FieldType::STRING_VIEW;
    }
    
    /**
     * @brief 检查字段值是否为布尔类型
     */
    constexpr bool is_bool() const noexcept {
        return field_type == FieldType::BOOL;
    }
    
    /**
     * @brief 获取字段值的字符串表示（用于调试）
     */
    std::string to_string() const {
        if constexpr (std::is_arithmetic_v<value_type>) {
            return std::to_string(value_);
        } else if constexpr (std::is_same_v<value_type, std::string>) {
            return value_;
        } else if constexpr (std::is_same_v<value_type, std::string_view>) {
            return std::string(value_);
        } else if constexpr (std::is_same_v<value_type, const char*>) {
            return std::string(value_);
        } else if constexpr (std::is_same_v<value_type, bool>) {
            return value_ ? "true" : "false";
        } else {
            return "custom_type";
        }
    }
};

/**
 * @brief 类型推导辅助函数
 * 
 * 使用示例：
 * auto field = make_field("user_id", 12345);
 * auto field2 = make_field("name", std::string("john"));
 */
template<typename T>
constexpr auto make_field(std::string_view key, T&& value) noexcept {
    return Field<T>(key, std::forward<T>(value));
}

/**
 * @brief 便捷的字段构造函数，支持类型推导
 */
#define FIELD(key, value) ::common::spdlog::structured::make_field(key, value)

/**
 * @brief 特殊字段类型定义
 */
namespace fields {
    
    /**
     * @brief 时间戳字段
     */
    inline auto timestamp(std::string_view key = "timestamp") {
        return make_field(key, std::chrono::system_clock::now());
    }
    
    /**
     * @brief 线程ID字段
     */
    inline auto thread_id(std::string_view key = "thread_id") {
        return make_field(key, std::hash<std::thread::id>{}(std::this_thread::get_id()));
    }
    
    /**
     * @brief 消息字段
     */
    inline auto message(std::string_view msg) {
        return make_field("message", msg);
    }
    
    /**
     * @brief 级别字段
     */
    inline auto level(std::string_view lvl) {
        return make_field("level", lvl);
    }
    
} // namespace fields

/**
 * @brief Field容器类型，用于存储多个字段
 */
template<typename... Fields>
struct FieldContainer {
    std::tuple<Fields...> fields;
    
    explicit FieldContainer(Fields&&... fields) 
        : fields(std::forward<Fields>(fields)...) {}
    
    template<std::size_t I>
    constexpr auto& get() const noexcept {
        return std::get<I>(fields);
    }
    
    constexpr std::size_t size() const noexcept {
        return sizeof...(Fields);
    }
};

/**
 * @brief 创建字段容器的便捷函数
 */
template<typename... Fields>
constexpr auto make_fields(Fields&&... fields) {
    return FieldContainer<Fields...>(std::forward<Fields>(fields)...);
}

} // namespace structured
} // namespace spdlog
} // namespace common

/**
 * @brief 为Field类型实现fmt::formatter
 * 这是Field类型能够被fmt格式化的关键
 */
template<typename T>
struct fmt::formatter<common::spdlog::structured::Field<T>> {
    // 解析格式规范
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin(), end = ctx.end();
        // 这里可以解析自定义格式选项，暂时保持简单
        if (it != end && *it != '}') {
            throw format_error("invalid format for Field");
        }
        return it;
    }
    
    // 格式化Field为JSON键值对格式
    template<typename FormatContext>
    auto format(const common::spdlog::structured::Field<T>& field, FormatContext& ctx) const -> decltype(ctx.out()) {
        using FieldType = common::spdlog::structured::FieldType;
        
        // 输出键名（总是使用引号）
        auto out = fmt::format_to(ctx.out(), "\"{}\":", field.key());
        
        // 根据类型输出值
        const auto& value = field.value();
        
        if constexpr (std::is_same_v<T, bool>) {
            return fmt::format_to(out, "{}", value ? "true" : "false");
        }
        else if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) {
            return fmt::format_to(out, "{}", value);
        }
        else if constexpr (std::is_same_v<T, std::string> || 
                          std::is_same_v<T, std::string_view> ||
                          std::is_same_v<T, const char*>) {
            // 字符串需要转义
            return fmt::format_to(out, "\"{}\"", value);
        }
        else if constexpr (std::is_same_v<T, std::chrono::system_clock::time_point>) {
            // 时间戳转换为毫秒
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                value.time_since_epoch()).count();
            return fmt::format_to(out, "{}", ms);
        }
        else {
            // 自定义类型，尝试转换为字符串
            return fmt::format_to(out, "\"{}\"", field.to_string());
        }
    }
};

/**
 * @brief 为FieldContainer实现fmt::formatter
 * 支持格式化多个字段为完整的JSON对象
 */
template<typename... Fields>
struct fmt::formatter<common::spdlog::structured::FieldContainer<Fields...>> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }
    
    template<typename FormatContext>
    auto format(const common::spdlog::structured::FieldContainer<Fields...>& container, FormatContext& ctx) const -> decltype(ctx.out()) {
        auto out = fmt::format_to(ctx.out(), "{{");
        
        // 使用fold expression展开所有字段
        bool first = true;
        std::apply([&](const auto&... fields) {
            ((first ? (first = false, fmt::format_to(out, "{}", fields)) 
                   : fmt::format_to(out, ",{}", fields)), ...);
        }, container.fields);
        
        return fmt::format_to(out, "}}");
    }
};

#include <thread>