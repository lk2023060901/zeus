#pragma once

#include <type_traits>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <stdexcept>

namespace common {
namespace utilities {
namespace detail {

/**
 * @brief 编译时false值，用于static_assert
 */
template<typename>
inline constexpr bool always_false_v = false;

/**
 * @brief 类型转换异常
 */
class TypeConversionException : public std::runtime_error {
public:
    explicit TypeConversionException(const std::string& message) 
        : std::runtime_error("Type conversion error: " + message) {}
};

/**
 * @brief 类型转换器特化模板
 * 
 * 为不同类型提供字符串转换功能。
 * 支持基础数据类型的高效转换。
 */
template<typename T>
struct TypeConverter {
    static T Convert(const std::string& str) {
        static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, std::string>, 
                     "Type must be arithmetic, string, or have specialized converter");
        
        if constexpr (std::is_same_v<T, std::string>) {
            return str;
        } else if constexpr (std::is_integral_v<T>) {
            return ConvertIntegral<T>(str);
        } else if constexpr (std::is_floating_point_v<T>) {
            return ConvertFloatingPoint<T>(str);
        } else {
            static_assert(always_false_v<T>, "Unsupported type for conversion");
        }
    }

private:
    template<typename U>
    static constexpr bool always_false_v = false;
    
    template<typename IntType>
    static IntType ConvertIntegral(const std::string& str) {
        try {
            if constexpr (std::is_same_v<IntType, bool>) {
                if (str == "true" || str == "1") return true;
                if (str == "false" || str == "0") return false;
                throw std::invalid_argument("Invalid boolean value");
            } else if constexpr (std::is_same_v<IntType, char>) {
                if (str.size() != 1) throw std::invalid_argument("String must be single character");
                return str[0];
            } else if constexpr (std::is_unsigned_v<IntType>) {
                if constexpr (sizeof(IntType) <= sizeof(unsigned long)) {
                    auto result = std::stoul(str);
                    if (result > std::numeric_limits<IntType>::max()) {
                        throw std::out_of_range("Value out of range");
                    }
                    return static_cast<IntType>(result);
                } else {
                    auto result = std::stoull(str);
                    if (result > std::numeric_limits<IntType>::max()) {
                        throw std::out_of_range("Value out of range");
                    }
                    return static_cast<IntType>(result);
                }
            } else {
                if constexpr (sizeof(IntType) <= sizeof(long)) {
                    auto result = std::stol(str);
                    if (result < std::numeric_limits<IntType>::min() || 
                        result > std::numeric_limits<IntType>::max()) {
                        throw std::out_of_range("Value out of range");
                    }
                    return static_cast<IntType>(result);
                } else {
                    auto result = std::stoll(str);
                    if (result < std::numeric_limits<IntType>::min() || 
                        result > std::numeric_limits<IntType>::max()) {
                        throw std::out_of_range("Value out of range");
                    }
                    return static_cast<IntType>(result);
                }
            }
        } catch (const std::exception& e) {
            throw TypeConversionException("Failed to convert '" + str + "' to integer type: " + e.what());
        }
    }
    
    template<typename FloatType>
    static FloatType ConvertFloatingPoint(const std::string& str) {
        try {
            if constexpr (std::is_same_v<FloatType, float>) {
                return std::stof(str);
            } else if constexpr (std::is_same_v<FloatType, double>) {
                return std::stod(str);
            } else if constexpr (std::is_same_v<FloatType, long double>) {
                return std::stold(str);
            } else {
                static_assert(always_false_v<FloatType>, "Unsupported floating point type");
            }
        } catch (const std::exception& e) {
            throw TypeConversionException("Failed to convert '" + str + "' to floating point type: " + e.what());
        }
    }
};

/**
 * @brief 容器类型特征检测
 */

// Vector类型检测
template<typename T>
struct is_vector : std::false_type {};

template<typename T, typename Alloc>
struct is_vector<std::vector<T, Alloc>> : std::true_type {
    using value_type = T;
    using allocator_type = Alloc;
};

template<typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

// Map类型检测
template<typename T>
struct is_map : std::false_type {};

template<typename K, typename V, typename Compare, typename Alloc>
struct is_map<std::map<K, V, Compare, Alloc>> : std::true_type {
    using key_type = K;
    using mapped_type = V;
    using key_compare = Compare;
    using allocator_type = Alloc;
};

template<typename T>
inline constexpr bool is_map_v = is_map<T>::value;

// UnorderedMap类型检测
template<typename T>
struct is_unordered_map : std::false_type {};

template<typename K, typename V, typename Hash, typename KeyEqual, typename Alloc>
struct is_unordered_map<std::unordered_map<K, V, Hash, KeyEqual, Alloc>> : std::true_type {
    using key_type = K;
    using mapped_type = V;
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Alloc;
};

template<typename T>
inline constexpr bool is_unordered_map_v = is_unordered_map<T>::value;

// 通用Map类型检测（包括map和unordered_map）
template<typename T>
struct is_map_like : std::bool_constant<is_map_v<T> || is_unordered_map_v<T>> {};

template<typename T>
inline constexpr bool is_map_like_v = is_map_like<T>::value;

/**
 * @brief 获取容器的键类型
 */
template<typename T>
struct map_key_type {
    static_assert(is_map_like_v<T>, "Type must be a map-like container");
};

template<typename K, typename V, typename Compare, typename Alloc>
struct map_key_type<std::map<K, V, Compare, Alloc>> {
    using type = K;
};

template<typename K, typename V, typename Hash, typename KeyEqual, typename Alloc>
struct map_key_type<std::unordered_map<K, V, Hash, KeyEqual, Alloc>> {
    using type = K;
};

template<typename T>
using map_key_type_t = typename map_key_type<T>::type;

/**
 * @brief 获取容器的值类型
 */
template<typename T>
struct map_value_type {
    static_assert(is_map_like_v<T>, "Type must be a map-like container");
};

template<typename K, typename V, typename Compare, typename Alloc>
struct map_value_type<std::map<K, V, Compare, Alloc>> {
    using type = V;
};

template<typename K, typename V, typename Hash, typename KeyEqual, typename Alloc>
struct map_value_type<std::unordered_map<K, V, Hash, KeyEqual, Alloc>> {
    using type = V;
};

template<typename T>
using map_value_type_t = typename map_value_type<T>::type;

/**
 * @brief 获取vector的元素类型
 */
template<typename T>
struct vector_value_type {
    static_assert(is_vector_v<T>, "Type must be a vector");
};

template<typename T, typename Alloc>
struct vector_value_type<std::vector<T, Alloc>> {
    using type = T;
};

template<typename T>
using vector_value_type_t = typename vector_value_type<T>::type;

/**
 * @brief 时间类型检测和转换
 */
template<typename T>
struct is_time_point : std::false_type {};

template<typename Clock, typename Duration>
struct is_time_point<std::chrono::time_point<Clock, Duration>> : std::true_type {
    using clock_type = Clock;
    using duration_type = Duration;
};

template<typename T>
inline constexpr bool is_time_point_v = is_time_point<T>::value;

/**
 * @brief 检测类型是否支持自定义转换
 */
template<typename T, typename = void>
struct has_from_string : std::false_type {};

template<typename T>
struct has_from_string<T, std::void_t<decltype(T::FromString(std::declval<std::string>()))>> 
    : std::true_type {};

template<typename T>
inline constexpr bool has_from_string_v = has_from_string<T>::value;

/**
 * @brief 智能类型转换器
 * 
 * 根据类型自动选择合适的转换方法。
 */
template<typename T>
struct SmartConverter {
    static T Convert(const std::string& str) {
        if constexpr (has_from_string_v<T>) {
            // 如果类型支持FromString静态方法，优先使用
            return T::FromString(str);
        } else {
            // 否则使用默认的TypeConverter
            return TypeConverter<T>::Convert(str);
        }
    }
};

/**
 * @brief 字符串特化（避免不必要的转换）
 */
template<>
struct SmartConverter<std::string> {
    static std::string Convert(const std::string& str) {
        return str;
    }
};

/**
 * @brief 便利的转换函数
 */
template<typename T>
T convert_from_string(const std::string& str) {
    return SmartConverter<T>::Convert(str);
}

/**
 * @brief 安全转换函数（不抛异常）
 */
template<typename T>
bool try_convert_from_string(const std::string& str, T& result) noexcept {
    try {
        result = SmartConverter<T>::Convert(str);
        return true;
    } catch (...) {
        return false;
    }
}

/**
 * @brief 带默认值的安全转换函数
 */
template<typename T>
T convert_from_string_or_default(const std::string& str, const T& default_value) noexcept {
    T result;
    if (try_convert_from_string(str, result)) {
        return result;
    }
    return default_value;
}

} // namespace detail
} // namespace utilities
} // namespace common