/**
 * @file custom_formatters_example.cpp
 * @brief 自定义格式化器示例
 * 
 * 这个示例演示了如何扩展Zeus结构化日志系统：
 * - 自定义Field类型及其formatter
 * - 自定义输出格式
 * - 复杂数据类型的序列化
 * - 特殊用途的日志格式化器
 */

#include "common/spdlog/zeus_structured_log.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include <map>
#include <iomanip>
#include <sstream>

using namespace common::spdlog::structured;

// ============================================================================
// 自定义数据类型定义
// ============================================================================

/**
 * @brief 用户信息结构体
 */
struct UserInfo {
    int64_t id;
    std::string name;
    std::string email;
    int age;
    bool active;
    
    UserInfo(int64_t id, std::string name, std::string email, int age, bool active)
        : id(id), name(std::move(name)), email(std::move(email)), age(age), active(active) {}
};

/**
 * @brief 地理位置信息
 */
struct GeoLocation {
    double latitude;
    double longitude;
    std::string country;
    std::string city;
    
    GeoLocation(double lat, double lon, std::string country, std::string city)
        : latitude(lat), longitude(lon), country(std::move(country)), city(std::move(city)) {}
};

/**
 * @brief HTTP请求详情
 */
struct HttpRequestDetail {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    size_t body_size;
    
    HttpRequestDetail(std::string method, std::string path, size_t body_size)
        : method(std::move(method)), path(std::move(path)), body_size(body_size) {}
};

/**
 * @brief 性能指标集合
 */
struct PerformanceMetrics {
    double cpu_usage;
    double memory_usage_mb;
    double disk_io_mbps;
    double network_io_mbps;
    int active_threads;
    
    PerformanceMetrics(double cpu, double mem, double disk, double net, int threads)
        : cpu_usage(cpu), memory_usage_mb(mem), disk_io_mbps(disk), network_io_mbps(net), active_threads(threads) {}
};

// ============================================================================
// 自定义Field类型的fmt::formatter特化
// ============================================================================

/**
 * @brief UserInfo类型的formatter - JSON格式
 */
template<>
struct fmt::formatter<Field<UserInfo>> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }
    
    template<typename FormatContext>
    auto format(const Field<UserInfo>& field, FormatContext& ctx) -> decltype(ctx.out()) {
        const auto& user = field.value();
        return fmt::format_to(ctx.out(), 
            "\"{}\":{{\"id\":{},\"name\":\"{}\",\"email\":\"{}\",\"age\":{},\"active\":{}}}",
            field.key(), user.id, user.name, user.email, user.age, user.active ? "true" : "false");
    }
};

/**
 * @brief GeoLocation类型的formatter
 */
template<>
struct fmt::formatter<Field<GeoLocation>> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }
    
    template<typename FormatContext>
    auto format(const Field<GeoLocation>& field, FormatContext& ctx) -> decltype(ctx.out()) {
        const auto& geo = field.value();
        return fmt::format_to(ctx.out(),
            "\"{}\":{{\"lat\":{:.6f},\"lon\":{:.6f},\"country\":\"{}\",\"city\":\"{}\"}}",
            field.key(), geo.latitude, geo.longitude, geo.country, geo.city);
    }
};

/**
 * @brief HttpRequestDetail类型的formatter
 */
template<>
struct fmt::formatter<Field<HttpRequestDetail>> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }
    
    template<typename FormatContext>
    auto format(const Field<HttpRequestDetail>& field, FormatContext& ctx) -> decltype(ctx.out()) {
        const auto& req = field.value();
        auto out = fmt::format_to(ctx.out(), "\"{}\":{{\"method\":\"{}\",\"path\":\"{}\",\"body_size\":{}",
                                 field.key(), req.method, req.path, req.body_size);
        
        // 添加headers
        if (!req.headers.empty()) {
            out = fmt::format_to(out, ",\"headers\":{{");
            bool first = true;
            for (const auto& [key, value] : req.headers) {
                if (!first) out = fmt::format_to(out, ",");
                out = fmt::format_to(out, "\"{}\":\"{}\"", key, value);
                first = false;
            }
            out = fmt::format_to(out, "}}");
        }
        
        // 添加query parameters
        if (!req.query_params.empty()) {
            out = fmt::format_to(out, ",\"query_params\":{{");
            bool first = true;
            for (const auto& [key, value] : req.query_params) {
                if (!first) out = fmt::format_to(out, ",");
                out = fmt::format_to(out, "\"{}\":\"{}\"", key, value);
                first = false;
            }
            out = fmt::format_to(out, "}}");
        }
        
        return fmt::format_to(out, "}}");
    }
};

/**
 * @brief PerformanceMetrics类型的formatter
 */
template<>
struct fmt::formatter<Field<PerformanceMetrics>> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        return ctx.begin();
    }
    
    template<typename FormatContext>
    auto format(const Field<PerformanceMetrics>& field, FormatContext& ctx) -> decltype(ctx.out()) {
        const auto& metrics = field.value();
        return fmt::format_to(ctx.out(),
            "\"{}\":{{\"cpu_usage\":{:.2f},\"memory_usage_mb\":{:.2f},\"disk_io_mbps\":{:.2f},\"network_io_mbps\":{:.2f},\"active_threads\":{}}}",
            field.key(), metrics.cpu_usage, metrics.memory_usage_mb, 
            metrics.disk_io_mbps, metrics.network_io_mbps, metrics.active_threads);
    }
};

// ============================================================================
// 自定义Field构造辅助函数
// ============================================================================

/**
 * @brief 创建UserInfo字段
 */
auto UserField(std::string_view key, const UserInfo& user) {
    return make_field(key, user);
}

/**
 * @brief 创建GeoLocation字段
 */
auto GeoField(std::string_view key, const GeoLocation& geo) {
    return make_field(key, geo);
}

/**
 * @brief 创建HttpRequestDetail字段
 */
auto HttpRequestField(std::string_view key, const HttpRequestDetail& req) {
    return make_field(key, req);
}

/**
 * @brief 创建PerformanceMetrics字段
 */
auto MetricsField(std::string_view key, const PerformanceMetrics& metrics) {
    return make_field(key, metrics);
}

// ============================================================================
// 自定义日志格式化器类
// ============================================================================

/**
 * @brief CSV格式日志格式化器
 */
class CSVStructuredLogger {
private:
    std::shared_ptr<::spdlog::logger> logger_;
    
public:
    explicit CSVStructuredLogger(std::shared_ptr<::spdlog::logger> logger) 
        : logger_(std::move(logger)) {}
    
    template<typename... Fields>
    void info_csv(Fields&&... fields) {
        if (!logger_ || !logger_->should_log(::spdlog::level::info)) {
            return;
        }
        
        std::ostringstream csv_line;
        csv_line << std::fixed << std::setprecision(3);
        
        // 添加时间戳
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        csv_line << std::put_time(std::gmtime(&time_t), "%Y-%m-%d %H:%M:%S");
        
        // 添加字段值
        ((csv_line << "," << format_csv_value(fields)), ...);
        
        logger_->info(csv_line.str());
    }
    
private:
    template<typename Field>
    std::string format_csv_value(const Field& field) {
        const auto& value = field.value();
        
        if constexpr (std::is_arithmetic_v<typename Field::value_type>) {
            return std::to_string(value);
        } else if constexpr (std::is_same_v<typename Field::value_type, std::string> ||
                            std::is_same_v<typename Field::value_type, std::string_view>) {
            // CSV字符串需要转义引号
            std::string escaped = "\"";
            for (char c : std::string(value)) {
                if (c == '"') escaped += "\"\"";  // 转义引号
                else escaped += c;
            }
            escaped += "\"";
            return escaped;
        } else if constexpr (std::is_same_v<typename Field::value_type, bool>) {
            return value ? "true" : "false";
        } else {
            return "\"" + field.to_string() + "\"";
        }
    }
};

/**
 * @brief 自定义XML格式日志记录器
 */
class XMLStructuredLogger {
private:
    std::shared_ptr<::spdlog::logger> logger_;
    
public:
    explicit XMLStructuredLogger(std::shared_ptr<::spdlog::logger> logger) 
        : logger_(std::move(logger)) {}
    
    template<typename... Fields>
    void info_xml(Fields&&... fields) {
        if (!logger_ || !logger_->should_log(::spdlog::level::info)) {
            return;
        }
        
        std::ostringstream xml_stream;
        xml_stream << "<log_entry>";
        
        // 添加时间戳
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        xml_stream << "<timestamp>" 
                  << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ") 
                  << "</timestamp>";
        
        // 添加字段
        ((xml_stream << format_xml_field(fields)), ...);
        
        xml_stream << "</log_entry>";
        logger_->info(xml_stream.str());
    }
    
private:
    template<typename Field>
    std::string format_xml_field(const Field& field) {
        std::string tag_name = std::string(field.key());
        // 简单的XML标签名清理
        std::replace(tag_name.begin(), tag_name.end(), ' ', '_');
        
        return "<" + tag_name + ">" + escape_xml(field.to_string()) + "</" + tag_name + ">";
    }
    
    std::string escape_xml(const std::string& str) {
        std::string escaped;
        escaped.reserve(str.size() * 1.1); // 预留空间
        
        for (char c : str) {
            switch (c) {
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '&': escaped += "&amp;"; break;
                case '"': escaped += "&quot;"; break;
                case '\'': escaped += "&apos;"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
};

// ============================================================================
// 演示函数
// ============================================================================

/**
 * @brief 演示自定义Field类型
 */
void demo_custom_field_types() {
    std::cout << "\n=== 自定义Field类型示例 ===" << std::endl;
    
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("custom_types");
    
    // 1. 用户信息字段
    UserInfo user(12345, "John Doe", "john@example.com", 28, true);
    logger->info(
        business_fields::event_type("user_profile_access"),
        UserField("user_info", user),
        fields::timestamp()
    );
    
    // 2. 地理位置字段
    GeoLocation location(37.7749, -122.4194, "USA", "San Francisco");
    logger->info(
        business_fields::event_type("user_location_update"),
        business_fields::user_id(12345),
        GeoField("location", location),
        fields::timestamp()
    );
    
    // 3. HTTP请求详情字段
    HttpRequestDetail request("POST", "/api/v1/users", 1024);
    request.headers["Content-Type"] = "application/json";
    request.headers["Authorization"] = "Bearer token123";
    request.query_params["version"] = "v1";
    request.query_params["format"] = "json";
    
    logger->info(
        business_fields::event_type("http_request_received"),
        business_fields::request_id("req_custom_123"),
        HttpRequestField("request_detail", request),
        fields::timestamp()
    );
    
    // 4. 性能指标字段
    PerformanceMetrics metrics(75.6, 1024.5, 45.8, 123.9, 8);
    logger->info(
        business_fields::event_type("system_performance_snapshot"),
        MetricsField("performance", metrics),
        FIELD("server_instance", "web-01"),
        fields::timestamp()
    );
}

/**
 * @brief 演示自定义格式化器
 */
void demo_custom_formatters() {
    std::cout << "\n=== 自定义格式化器示例 ===" << std::endl;
    
    // 1. CSV格式日志器
    auto csv_logger = CSVStructuredLogger(ZEUS_GET_LOGGER("csv_format"));
    
    std::cout << "CSV格式输出示例：" << std::endl;
    csv_logger.info_csv(
        FIELD("user_id", 12345),
        FIELD("action", "login"),
        FIELD("duration_ms", 234.567),
        FIELD("success", true)
    );
    
    csv_logger.info_csv(
        FIELD("user_id", 54321),
        FIELD("action", "logout"),
        FIELD("duration_ms", 12.345),
        FIELD("success", true)
    );
    
    // 2. XML格式日志器
    auto xml_logger = XMLStructuredLogger(ZEUS_GET_LOGGER("xml_format"));
    
    std::cout << "\nXML格式输出示例：" << std::endl;
    xml_logger.info_xml(
        FIELD("transaction_id", "txn_123456"),
        FIELD("amount", 99.99),
        FIELD("currency", "USD"),
        FIELD("status", "completed")
    );
    
    xml_logger.info_xml(
        FIELD("error_code", "VALIDATION_FAILED"),
        FIELD("error_message", "Invalid email format: test@"),
        FIELD("field_name", "email"),
        FIELD("retry_count", 0)
    );
}

/**
 * @brief 演示复杂嵌套数据结构
 */
void demo_complex_nested_data() {
    std::cout << "\n=== 复杂嵌套数据结构示例 ===" << std::endl;
    
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("complex_data");
    
    // 创建复杂的用户会话数据
    UserInfo session_user(98765, "Alice Smith", "alice@example.com", 32, true);
    GeoLocation session_location(40.7128, -74.0060, "USA", "New York");
    
    // HTTP请求详情
    HttpRequestDetail api_request("GET", "/api/v1/dashboard", 0);
    api_request.headers["User-Agent"] = "Mozilla/5.0 (Windows NT 10.0; Win64; x64)";
    api_request.headers["Accept"] = "application/json";
    api_request.headers["Accept-Language"] = "en-US,en;q=0.9";
    api_request.query_params["page"] = "1";
    api_request.query_params["limit"] = "20";
    api_request.query_params["sort"] = "created_at";
    
    // 性能数据
    PerformanceMetrics session_metrics(23.4, 512.8, 12.5, 67.8, 4);
    
    // 记录完整的用户会话日志
    logger->info(
        business_fields::event_type("user_session_activity"),
        business_fields::request_id("req_session_789abc"),
        business_fields::correlation_id("corr_user_session_123"),
        UserField("session_user", session_user),
        GeoField("client_location", session_location),
        HttpRequestField("request_details", api_request),
        MetricsField("server_metrics", session_metrics),
        FIELD("session_duration_minutes", 45.5),
        FIELD("pages_viewed", 12),
        FIELD("actions_performed", 8),
        fields::timestamp("session_start"),
        fields::thread_id()
    );
}

/**
 * @brief 演示条件格式化
 */
void demo_conditional_formatting() {
    std::cout << "\n=== 条件格式化示例 ===" << std::endl;
    
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("conditional");
    
    // 模拟不同条件下的日志格式
    std::vector<std::pair<std::string, int>> operations = {
        {"fast_operation", 50},
        {"normal_operation", 150},
        {"slow_operation", 800},
        {"very_slow_operation", 2500}
    };
    
    for (const auto& [op_name, duration] : operations) {
        if (duration < 100) {
            // 快速操作 - 简单格式
            logger->debug(
                FIELD("operation", op_name),
                FIELD("duration_ms", duration),
                FIELD("performance_level", "excellent")
            );
        } else if (duration < 500) {
            // 正常操作 - 标准格式
            logger->info(
                business_fields::event_type("operation_completed"),
                business_fields::operation(op_name),
                business_fields::processing_time_ms(duration),
                FIELD("performance_level", "good")
            );
        } else if (duration < 1000) {
            // 慢操作 - 详细格式
            logger->warn(
                business_fields::event_type("slow_operation_detected"),
                business_fields::operation(op_name),
                business_fields::processing_time_ms(duration),
                FIELD("performance_level", "poor"),
                FIELD("optimization_suggested", true),
                fields::timestamp("detected_at")
            );
        } else {
            // 非常慢的操作 - 告警格式
            logger->error(
                business_fields::event_type("critical_performance_issue"),
                business_fields::operation(op_name),
                business_fields::processing_time_ms(duration),
                business_fields::error_code("PERF_001"),
                business_fields::error_message("Operation exceeded acceptable duration threshold"),
                FIELD("performance_level", "critical"),
                FIELD("threshold_ms", 1000),
                FIELD("immediate_action_required", true),
                fields::timestamp("issue_detected_at"),
                fields::thread_id()
            );
        }
    }
}

/**
 * @brief 主函数
 */
int main() {
    try {
        std::cout << "=== Zeus结构化日志自定义格式化器示例 ===" << std::endl;
        
        // 创建日志目录
        std::filesystem::create_directories("logs");
        
        // 初始化结构化日志系统
        if (!InitializeStructuredLogging("", OutputFormat::JSON)) {
            std::cerr << "Failed to initialize structured logging" << std::endl;
            return 1;
        }
        
        std::cout << "使用Zeus结构化日志框架版本: " << GetVersion() << std::endl;
        
        // 运行各种自定义格式化演示
        demo_custom_field_types();
        demo_custom_formatters();
        demo_complex_nested_data();
        demo_conditional_formatting();
        
        std::cout << "\n=== 自定义格式化器示例完成 ===" << std::endl;
        std::cout << "这个示例展示了如何扩展Zeus结构化日志系统：" << std::endl;
        std::cout << "- 为自定义数据类型创建Field formatter" << std::endl;
        std::cout << "- 实现自定义输出格式（CSV、XML等）" << std::endl;
        std::cout << "- 处理复杂嵌套数据结构" << std::endl;
        std::cout << "- 根据条件选择不同的日志格式" << std::endl;
        std::cout << "- 优化特定业务场景的日志记录" << std::endl;
        
        // 关闭日志系统
        ShutdownStructuredLogging();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "自定义格式化器示例错误: " << e.what() << std::endl;
        return 1;
    }
}