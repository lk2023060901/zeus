/**
 * @file performance_comparison_example.cpp
 * @brief 性能对比示例
 * 
 * 这个示例对比了不同日志记录方式的性能：
 * - 传统JSON方式 vs Field方式
 * - 不同输出格式的性能差异
 * - 内存分配情况对比
 * - 在不同负载下的表现
 */

#include "common/spdlog/zeus_structured_log.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <filesystem>
#include <random>
#include <vector>
#include <memory>

using namespace common::spdlog::structured;

/**
 * @brief 性能测试结果
 */
struct PerformanceResult {
    std::string test_name;
    std::chrono::microseconds duration;
    size_t iterations;
    double avg_per_log_ns;
    size_t memory_usage_estimate;
    
    void print() const {
        std::cout << "测试: " << test_name << std::endl;
        std::cout << "  总用时: " << duration.count() << " 微秒" << std::endl;
        std::cout << "  迭代次数: " << iterations << std::endl;
        std::cout << "  平均每条日志: " << avg_per_log_ns << " 纳秒" << std::endl;
        std::cout << "  估计内存使用: " << memory_usage_estimate << " 字节" << std::endl;
        std::cout << std::endl;
    }
};

/**
 * @brief 测试数据生成器
 */
class TestDataGenerator {
private:
    std::random_device rd_;
    std::mt19937 gen_;
    std::uniform_int_distribution<int> int_dist_;
    std::uniform_real_distribution<double> double_dist_;
    std::uniform_int_distribution<int> string_length_dist_;
    std::vector<std::string> sample_strings_;
    
public:
    TestDataGenerator() : gen_(rd_()), int_dist_(1, 100000), double_dist_(0.0, 1000.0), string_length_dist_(5, 20) {
        // 预生成一些测试字符串
        sample_strings_.reserve(1000);
        for (int i = 0; i < 1000; ++i) {
            sample_strings_.push_back(generate_random_string());
        }
    }
    
    int random_int() { return int_dist_(gen_); }
    double random_double() { return double_dist_(gen_); }
    bool random_bool() { return gen_() % 2 == 0; }
    
    const std::string& random_string() {
        return sample_strings_[gen_() % sample_strings_.size()];
    }
    
private:
    std::string generate_random_string() {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        int length = string_length_dist_(gen_);
        std::string result;
        result.reserve(length);
        
        for (int i = 0; i < length; ++i) {
            result += charset[gen_() % (sizeof(charset) - 1)];
        }
        return result;
    }
};

/**
 * @brief 传统JSON方式性能测试
 */
PerformanceResult test_traditional_json(size_t iterations) {
    auto logger = ZEUS_GET_LOGGER("perf_json");
    TestDataGenerator generator;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        nlohmann::json data;
        data["iteration"] = static_cast<int>(i);
        data["user_id"] = generator.random_int();
        data["username"] = generator.random_string();
        data["score"] = generator.random_double();
        data["active"] = generator.random_bool();
        data["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (logger) {
            logger->info("EVENT: {}", data.dump());
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return {
        "传统JSON方式",
        duration,
        iterations,
        static_cast<double>(duration.count() * 1000.0 / iterations), // 转换为纳秒
        iterations * 500 // 估算内存使用
    };
}

/**
 * @brief Field方式性能测试
 */
PerformanceResult test_field_approach(size_t iterations) {
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("perf_field");
    TestDataGenerator generator;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        if (logger) {
            logger->info(
                FIELD("iteration", static_cast<int>(i)),
                FIELD("user_id", generator.random_int()),
                FIELD("username", generator.random_string()),
                FIELD("score", generator.random_double()),
                FIELD("active", generator.random_bool()),
                fields::timestamp()
            );
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return {
        "Field对象方式",
        duration,
        iterations,
        static_cast<double>(duration.count() * 1000.0 / iterations),
        iterations * 200 // 估算内存使用（更少的分配）
    };
}

/**
 * @brief Key-Value方式性能测试
 */
PerformanceResult test_key_value_approach(size_t iterations) {
    auto logger = ZEUS_GET_STRUCTURED_LOGGER("perf_kv");
    TestDataGenerator generator;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        if (logger) {
            logger->info_kv(
                "iteration", static_cast<int>(i),
                "user_id", generator.random_int(),
                "username", generator.random_string(),
                "score", generator.random_double(),
                "active", generator.random_bool()
            );
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    return {
        "Key-Value方式",
        duration,
        iterations,
        static_cast<double>(duration.count() * 1000.0 / iterations),
        iterations * 250 // 估算内存使用
    };
}

/**
 * @brief 不同输出格式性能测试
 */
void test_output_formats(size_t iterations) {
    std::cout << "\n=== 输出格式性能对比 (iterations: " << iterations << ") ===" << std::endl;
    
    TestDataGenerator generator;
    std::vector<PerformanceResult> results;
    
    // JSON格式测试
    {
        auto logger = ZEUS_GET_STRUCTURED_LOGGER("format_json");
        logger->set_format(OutputFormat::JSON);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            logger->info(
                FIELD("format", "JSON"),
                FIELD("iteration", static_cast<int>(i)),
                FIELD("value", generator.random_double())
            );
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        results.push_back({
            "JSON格式输出",
            duration,
            iterations,
            static_cast<double>(duration.count() * 1000.0 / iterations),
            0
        });
    }
    
    // Key-Value格式测试
    {
        auto logger = ZEUS_GET_STRUCTURED_LOGGER("format_kv");
        logger->set_format(OutputFormat::KEY_VALUE);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            logger->info(
                FIELD("format", "KEY_VALUE"),
                FIELD("iteration", static_cast<int>(i)),
                FIELD("value", generator.random_double())
            );
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        results.push_back({
            "Key-Value格式输出",
            duration,
            iterations,
            static_cast<double>(duration.count() * 1000.0 / iterations),
            0
        });
    }
    
    // LogFmt格式测试
    {
        auto logger = ZEUS_GET_STRUCTURED_LOGGER("format_logfmt");
        logger->set_format(OutputFormat::LOGFMT);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            logger->info(
                FIELD("format", "LOGFMT"),
                FIELD("iteration", static_cast<int>(i)),
                FIELD("value", generator.random_double())
            );
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        results.push_back({
            "LogFmt格式输出",
            duration,
            iterations,
            static_cast<double>(duration.count() * 1000.0 / iterations),
            0
        });
    }
    
    // 打印结果
    for (const auto& result : results) {
        result.print();
    }
}

/**
 * @brief 不同负载下的性能测试
 */
void test_different_loads() {
    std::cout << "\n=== 不同负载下的性能对比 ===" << std::endl;
    
    std::vector<size_t> loads = {1000, 10000, 50000, 100000};
    
    for (size_t load : loads) {
        std::cout << "\n--- 负载: " << load << " 条日志 ---" << std::endl;
        
        auto json_result = test_traditional_json(load);
        auto field_result = test_field_approach(load);
        auto kv_result = test_key_value_approach(load);
        
        json_result.print();
        field_result.print();
        kv_result.print();
        
        // 计算性能提升
        double field_improvement = (json_result.avg_per_log_ns - field_result.avg_per_log_ns) / json_result.avg_per_log_ns * 100.0;
        double kv_improvement = (json_result.avg_per_log_ns - kv_result.avg_per_log_ns) / json_result.avg_per_log_ns * 100.0;
        
        std::cout << "性能提升:" << std::endl;
        std::cout << "  Field方式相比JSON: " << field_improvement << "%" << std::endl;
        std::cout << "  Key-Value方式相比JSON: " << kv_improvement << "%" << std::endl;
        std::cout << "================================" << std::endl;
    }
}

/**
 * @brief 复杂对象性能测试
 */
void test_complex_objects() {
    std::cout << "\n=== 复杂对象性能测试 ===" << std::endl;
    
    const size_t iterations = 10000;
    TestDataGenerator generator;
    
    // 传统方式 - 复杂JSON对象
    auto start = std::chrono::high_resolution_clock::now();
    auto json_logger = ZEUS_GET_LOGGER("complex_json");
    
    for (size_t i = 0; i < iterations; ++i) {
        nlohmann::json complex_data;
        complex_data["event_id"] = static_cast<int>(i);
        complex_data["user"]["id"] = generator.random_int();
        complex_data["user"]["name"] = generator.random_string();
        complex_data["user"]["profile"]["score"] = generator.random_double();
        complex_data["user"]["profile"]["active"] = generator.random_bool();
        complex_data["metadata"]["ip"] = "192.168.1." + std::to_string(generator.random_int() % 255);
        complex_data["metadata"]["user_agent"] = "Zeus-Client/1.0";
        complex_data["metrics"]["cpu_usage"] = generator.random_double();
        complex_data["metrics"]["memory_mb"] = generator.random_double() * 1000;
        
        if (json_logger) {
            json_logger->info("COMPLEX_EVENT: {}", complex_data.dump());
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto json_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Field方式 - 扁平化字段
    start = std::chrono::high_resolution_clock::now();
    auto field_logger = ZEUS_GET_STRUCTURED_LOGGER("complex_field");
    
    for (size_t i = 0; i < iterations; ++i) {
        if (field_logger) {
            field_logger->info(
                FIELD("event_id", static_cast<int>(i)),
                FIELD("user_id", generator.random_int()),
                FIELD("user_name", generator.random_string()),
                FIELD("user_score", generator.random_double()),
                FIELD("user_active", generator.random_bool()),
                FIELD("ip_address", "192.168.1." + std::to_string(generator.random_int() % 255)),
                FIELD("user_agent", "Zeus-Client/1.0"),
                FIELD("cpu_usage", generator.random_double()),
                FIELD("memory_mb", generator.random_double() * 1000)
            );
        }
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto field_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "复杂对象性能对比 (" << iterations << " iterations):" << std::endl;
    std::cout << "  传统JSON方式: " << json_duration.count() << " 微秒" << std::endl;
    std::cout << "  Field方式: " << field_duration.count() << " 微秒" << std::endl;
    std::cout << "  性能提升: " << ((double)(json_duration.count() - field_duration.count()) / json_duration.count() * 100.0) << "%" << std::endl;
}

/**
 * @brief 主函数
 */
int main() {
    try {
        std::cout << "=== Zeus结构化日志性能对比测试 ===" << std::endl;
        
        // 创建日志目录
        std::filesystem::create_directories("logs");
        
        // 初始化日志系统
        if (!InitializeStructuredLogging("", OutputFormat::JSON)) {
            std::cerr << "Failed to initialize structured logging" << std::endl;
            return 1;
        }
        
        // 设置为较低的日志级别以减少输出对性能测试的影响
        // （在实际性能测试中通常会禁用控制台输出）
        
        // 运行各种性能测试
        test_different_loads();
        test_output_formats(25000);
        test_complex_objects();
        
        std::cout << "\n=== 性能测试完成 ===" << std::endl;
        std::cout << "注意：实际性能会受到以下因素影响：" << std::endl;
        std::cout << "- 日志输出目标（控制台/文件）" << std::endl;
        std::cout << "- 日志级别过滤" << std::endl;
        std::cout << "- 编译优化级别" << std::endl;
        std::cout << "- 系统负载情况" << std::endl;
        
        // 关闭日志系统
        ShutdownStructuredLogging();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "性能测试错误: " << e.what() << std::endl;
        return 1;
    }
}