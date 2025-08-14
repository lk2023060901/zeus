#pragma once

#include "singleton.h"
#include "type_traits_utils.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <optional>

namespace common {
namespace utilities {

/**
 * @brief 默认分隔符定义
 * 
 * 选择不受中英文输入法影响的安全分隔符。
 */
struct DefaultDelimiters {
    // 一级推荐：最安全，最常用
    static constexpr const char* PRIMARY = "-";      // 连字符
    static constexpr const char* PIPE = "|";         // 竖线
    static constexpr const char* UNDERSCORE = "_";   // 下划线
    
    // 二级推荐：安全，但可能在某些场景下有歧义
    static constexpr const char* SLASH = "/";        // 斜杠
    static constexpr const char* STAR = "*";         // 星号
    static constexpr const char* PLUS = "+";         // 加号
    static constexpr const char* EQUAL = "=";        // 等号
    static constexpr const char* HASH = "#";         // 井号
    static constexpr const char* AT = "@";           // At符号
    
    // 三级推荐：特殊用途
    static constexpr const char* TAB = "\t";         // Tab制表符
    static constexpr const char* SPACE = " ";        // 空格
    static constexpr const char* NEWLINE = "\n";     // 换行符
    
    // 键值对分隔符
    static constexpr const char* KV_SEPARATOR = ":"; // 键值分隔符
    static constexpr const char* PAIR_SEPARATOR = ","; // 对分隔符
};

/**
 * @brief 字符串处理工具类（非线程安全版本）
 * 
 * 使用单例模式，默认非线程安全以获得最佳性能。
 * 如需线程安全版本，请使用ThreadSafeStringUtils。
 */
class StringUtils : public Singleton<StringUtils, NullMutex> {
    SINGLETON_FACTORY(StringUtils);
    SINGLETON_ACCESSOR(StringUtils);

public:
    // ============ 基础字符串操作 ============
    
    /**
     * @brief 分割字符串
     * @param str 要分割的字符串
     * @param delimiter 分隔符，默认为"-"
     * @param skip_empty 是否跳过空字符串，默认true
     * @return 分割后的字符串向量
     */
    std::vector<std::string> Split(const std::string& str, 
                                  const std::string& delimiter = DefaultDelimiters::PRIMARY,
                                  bool skip_empty = true) const;
    
    /**
     * @brief 连接字符串向量
     * @param parts 要连接的字符串向量
     * @param delimiter 连接符，默认为"-"
     * @return 连接后的字符串
     */
    std::string Join(const std::vector<std::string>& parts,
                    const std::string& delimiter = DefaultDelimiters::PRIMARY) const;
    
    /**
     * @brief 去除字符串两端空白字符
     * @param str 要处理的字符串
     * @param whitespace 要去除的字符集，默认为空白字符
     * @return 处理后的字符串
     */
    std::string Trim(const std::string& str, const std::string& whitespace = " \t\n\r") const;
    
    /**
     * @brief 智能分隔符检测
     * @param str 要检测的字符串
     * @return 检测到的最佳分隔符
     */
    std::string DetectDelimiter(const std::string& str) const;
    
    // ============ 类型安全的容器解析 ============
    
    /**
     * @brief 解析字符串到向量（引用版本）
     * @tparam T 向量元素类型
     * @param str 要解析的字符串
     * @param result 输出向量
     * @param delimiter 分隔符
     */
    template<typename T>
    void ParseToVector(const std::string& str, 
                      std::vector<T>& result, 
                      const std::string& delimiter = DefaultDelimiters::PRIMARY) const;
    
    /**
     * @brief 解析字符串到向量（返回值版本）
     * @tparam T 向量元素类型
     * @param str 要解析的字符串
     * @param delimiter 分隔符
     * @return 解析后的向量
     */
    template<typename T>
    std::vector<T> ParseToVector(const std::string& str,
                                const std::string& delimiter = DefaultDelimiters::PRIMARY) const;
    
    /**
     * @brief 解析字符串到映射（引用版本）
     * @tparam K 键类型
     * @tparam V 值类型
     * @param str 要解析的字符串
     * @param result 输出映射
     * @param pair_delimiter 键值对分隔符
     * @param kv_delimiter 键值分隔符
     */
    template<typename K, typename V>
    void ParseToMap(const std::string& str,
                   std::map<K, V>& result,
                   const std::string& pair_delimiter = DefaultDelimiters::PAIR_SEPARATOR,
                   const std::string& kv_delimiter = DefaultDelimiters::KV_SEPARATOR) const;
    
    /**
     * @brief 解析字符串到映射（返回值版本）
     * @tparam K 键类型
     * @tparam V 值类型
     * @param str 要解析的字符串
     * @param pair_delimiter 键值对分隔符
     * @param kv_delimiter 键值分隔符
     * @return 解析后的映射
     */
    template<typename K, typename V>
    std::map<K, V> ParseToMap(const std::string& str,
                             const std::string& pair_delimiter = DefaultDelimiters::PAIR_SEPARATOR,
                             const std::string& kv_delimiter = DefaultDelimiters::KV_SEPARATOR) const;
    
    /**
     * @brief 解析字符串到无序映射
     * @tparam K 键类型
     * @tparam V 值类型
     * @param str 要解析的字符串
     * @param result 输出无序映射
     * @param pair_delimiter 键值对分隔符
     * @param kv_delimiter 键值分隔符
     */
    template<typename K, typename V>
    void ParseToUnorderedMap(const std::string& str,
                            std::unordered_map<K, V>& result,
                            const std::string& pair_delimiter = DefaultDelimiters::PAIR_SEPARATOR,
                            const std::string& kv_delimiter = DefaultDelimiters::KV_SEPARATOR) const;
    
    // ============ 智能容器解析（自动类型推导） ============
    
    /**
     * @brief 智能解析到容器
     * @tparam Container 容器类型（自动推导）
     * @param str 要解析的字符串
     * @param delimiter 分隔符
     * @return 解析后的容器
     */
    template<typename Container>
    Container Parse(const std::string& str, 
                   const std::string& delimiter = DefaultDelimiters::PRIMARY) const;
    
    // ============ 安全解析（不抛异常） ============
    
    /**
     * @brief 安全解析到向量
     * @tparam T 向量元素类型
     * @param str 要解析的字符串
     * @param result 输出向量
     * @param delimiter 分隔符
     * @return 是否解析成功
     */
    template<typename T>
    bool TryParseToVector(const std::string& str,
                         std::vector<T>& result,
                         const std::string& delimiter = DefaultDelimiters::PRIMARY) const noexcept;
    
    /**
     * @brief 带默认值的安全解析
     * @tparam T 向量元素类型
     * @param str 要解析的字符串
     * @param default_value 默认值
     * @param delimiter 分隔符
     * @return 解析结果或默认值
     */
    template<typename T>
    std::vector<T> ParseToVectorSafe(const std::string& str,
                                    const std::vector<T>& default_value = {},
                                    const std::string& delimiter = DefaultDelimiters::PRIMARY) const noexcept;
    
    // ============ 日期时间处理 ============
    
    /**
     * @brief 时间点转字符串
     * @param tp 时间点
     * @param format 格式字符串，默认ISO格式
     * @return 时间字符串
     */
    std::string TimeToString(const std::chrono::system_clock::time_point& tp,
                            const std::string& format = "%Y-%m-%d %H:%M:%S") const;
    
    /**
     * @brief 字符串转时间点
     * @param str 时间字符串
     * @param format 格式字符串，默认ISO格式
     * @return 时间点
     */
    std::chrono::system_clock::time_point StringToTime(const std::string& str,
                                                      const std::string& format = "%Y-%m-%d %H:%M:%S") const;
    
    /**
     * @brief 安全字符串转时间点
     * @param str 时间字符串
     * @param result 输出时间点
     * @param format 格式字符串
     * @return 是否转换成功
     */
    bool TryStringToTime(const std::string& str,
                        std::chrono::system_clock::time_point& result,
                        const std::string& format = "%Y-%m-%d %H:%M:%S") const noexcept;
    
    // ============ 输入法兼容性处理 ============
    
    /**
     * @brief 检测字符串中是否包含中文标点符号
     * @param str 要检测的字符串
     * @return 是否包含中文标点
     */
    bool HasChinesePunctuation(const std::string& str) const;
    
    /**
     * @brief 标准化标点符号（中文转英文）
     * @param str 要处理的字符串
     * @return 标准化后的字符串
     */
    std::string NormalizePunctuation(const std::string& str) const;
    
    // ============ 高级功能 ============
    
    /**
     * @brief 批量解析多个字符串
     * @tparam T 目标类型
     * @param strings 字符串向量
     * @param delimiter 分隔符
     * @return 解析结果向量
     */
    template<typename T>
    std::vector<std::vector<T>> BatchParseToVector(const std::vector<std::string>& strings,
                                                  const std::string& delimiter = DefaultDelimiters::PRIMARY) const;

private:
    StringUtils() = default;
    ~StringUtils() = default;
    
    // 允许Singleton访问私有构造函数和析构函数
    friend class Singleton<StringUtils>;
    friend class std::default_delete<StringUtils>;
    
    // 内部辅助方法
    std::vector<std::pair<std::string, std::string>> GetChinesePunctuationReplacements() const;
};

/**
 * @brief 线程安全的字符串处理工具类
 * 
 * 与StringUtils提供相同的接口，但内部使用线程安全的单例实现。
 */
class ThreadSafeStringUtils : public Singleton<ThreadSafeStringUtils, ThreadSafeMutex> {
    THREAD_SAFE_SINGLETON_FACTORY(ThreadSafeStringUtils);
    SINGLETON_ACCESSOR(ThreadSafeStringUtils);

public:
    // 提供与StringUtils相同的接口
    // 为了避免代码重复，这里声明主要方法，实现将委托给StringUtils
    
    std::vector<std::string> Split(const std::string& str, 
                                  const std::string& delimiter = DefaultDelimiters::PRIMARY,
                                  bool skip_empty = true) const;
    
    std::string Join(const std::vector<std::string>& parts,
                    const std::string& delimiter = DefaultDelimiters::PRIMARY) const;
    
    template<typename T>
    void ParseToVector(const std::string& str, 
                      std::vector<T>& result, 
                      const std::string& delimiter = DefaultDelimiters::PRIMARY) const;
    
    template<typename T>
    std::vector<T> ParseToVector(const std::string& str,
                                const std::string& delimiter = DefaultDelimiters::PRIMARY) const;
    
    template<typename K, typename V>
    void ParseToMap(const std::string& str,
                   std::map<K, V>& result,
                   const std::string& pair_delimiter = DefaultDelimiters::PAIR_SEPARATOR,
                   const std::string& kv_delimiter = DefaultDelimiters::KV_SEPARATOR) const;
    
    std::string TimeToString(const std::chrono::system_clock::time_point& tp,
                            const std::string& format = "%Y-%m-%d %H:%M:%S") const;
    
    std::chrono::system_clock::time_point StringToTime(const std::string& str,
                                                      const std::string& format = "%Y-%m-%d %H:%M:%S") const;

private:
    ThreadSafeStringUtils() = default;
    ~ThreadSafeStringUtils() = default;
    
    // 允许Singleton访问私有构造函数和析构函数
    friend class Singleton<ThreadSafeStringUtils, ThreadSafeMutex>;
    friend class std::default_delete<ThreadSafeStringUtils>;
};

// ============ 模板方法实现 ============

template<typename T>
void StringUtils::ParseToVector(const std::string& str, 
                               std::vector<T>& result, 
                               const std::string& delimiter) const {
    result.clear();
    auto parts = Split(str, delimiter);
    result.reserve(parts.size());
    
    for (const auto& part : parts) {
        if (!part.empty()) {
            result.emplace_back(detail::convert_from_string<T>(part));
        }
    }
}

template<typename T>
std::vector<T> StringUtils::ParseToVector(const std::string& str,
                                         const std::string& delimiter) const {
    std::vector<T> result;
    ParseToVector(str, result, delimiter);
    return result;
}

template<typename K, typename V>
void StringUtils::ParseToMap(const std::string& str,
                            std::map<K, V>& result,
                            const std::string& pair_delimiter,
                            const std::string& kv_delimiter) const {
    result.clear();
    
    auto pairs = Split(str, pair_delimiter);
    
    for (const auto& pair : pairs) {
        if (pair.empty()) continue;
        
        auto kv_parts = Split(pair, kv_delimiter);
        if (kv_parts.size() >= 2) {
            K key = detail::convert_from_string<K>(kv_parts[0]);
            V value = detail::convert_from_string<V>(kv_parts[1]);
            result.emplace(std::move(key), std::move(value));
        }
    }
}

template<typename K, typename V>
std::map<K, V> StringUtils::ParseToMap(const std::string& str,
                                      const std::string& pair_delimiter,
                                      const std::string& kv_delimiter) const {
    std::map<K, V> result;
    ParseToMap(str, result, pair_delimiter, kv_delimiter);
    return result;
}

template<typename K, typename V>
void StringUtils::ParseToUnorderedMap(const std::string& str,
                                     std::unordered_map<K, V>& result,
                                     const std::string& pair_delimiter,
                                     const std::string& kv_delimiter) const {
    result.clear();
    
    auto pairs = Split(str, pair_delimiter);
    
    for (const auto& pair : pairs) {
        if (pair.empty()) continue;
        
        auto kv_parts = Split(pair, kv_delimiter);
        if (kv_parts.size() >= 2) {
            K key = detail::convert_from_string<K>(kv_parts[0]);
            V value = detail::convert_from_string<V>(kv_parts[1]);
            result.emplace(std::move(key), std::move(value));
        }
    }
}

template<typename Container>
Container StringUtils::Parse(const std::string& str, const std::string& delimiter) const {
    Container result;
    
    if constexpr (detail::is_vector_v<Container>) {
        using ValueType = detail::vector_value_type_t<Container>;
        ParseToVector<ValueType>(str, result, delimiter);
    } else if constexpr (detail::is_map_v<Container>) {
        static_assert(detail::always_false_v<Container>, 
                     "Map parsing requires explicit ParseToMap call with delimiters");
    } else {
        static_assert(detail::always_false_v<Container>, "Unsupported container type");
    }
    
    return result;
}

template<typename T>
bool StringUtils::TryParseToVector(const std::string& str,
                                  std::vector<T>& result,
                                  const std::string& delimiter) const noexcept {
    try {
        ParseToVector(str, result, delimiter);
        return true;
    } catch (...) {
        result.clear();
        return false;
    }
}

template<typename T>
std::vector<T> StringUtils::ParseToVectorSafe(const std::string& str,
                                             const std::vector<T>& default_value,
                                             const std::string& delimiter) const noexcept {
    std::vector<T> result;
    if (TryParseToVector(str, result, delimiter)) {
        return result;
    }
    return default_value;
}

template<typename T>
std::vector<std::vector<T>> StringUtils::BatchParseToVector(const std::vector<std::string>& strings,
                                                           const std::string& delimiter) const {
    std::vector<std::vector<T>> results;
    results.reserve(strings.size());
    
    for (const auto& str : strings) {
        results.emplace_back(ParseToVector<T>(str, delimiter));
    }
    
    return results;
}

// ThreadSafeStringUtils模板方法实现
template<typename T>
void ThreadSafeStringUtils::ParseToVector(const std::string& str, 
                                         std::vector<T>& result, 
                                         const std::string& delimiter) const {
    // 委托给非线程安全版本，因为字符串解析本身是线程安全的
    StringUtils::GetInstance().ParseToVector(str, result, delimiter);
}

template<typename T>
std::vector<T> ThreadSafeStringUtils::ParseToVector(const std::string& str,
                                                   const std::string& delimiter) const {
    return StringUtils::GetInstance().ParseToVector<T>(str, delimiter);
}

template<typename K, typename V>
void ThreadSafeStringUtils::ParseToMap(const std::string& str,
                                      std::map<K, V>& result,
                                      const std::string& pair_delimiter,
                                      const std::string& kv_delimiter) const {
    StringUtils::GetInstance().ParseToMap(str, result, pair_delimiter, kv_delimiter);
}

} // namespace utilities
} // namespace common