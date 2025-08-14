/**
 * @file test_string_utils_advanced.cpp
 * @brief Zeus StringUtils高级功能测试
 */

#include "common/utilities/string_utils.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <future>
#include <string>

using namespace common::utilities;

/**
 * @brief StringUtils高级功能测试类
 */
class StringUtilsAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override {
        StringUtils::Reset();
        ThreadSafeStringUtils::Reset();
    }
    
    void TearDown() override {
        StringUtils::Reset();
        ThreadSafeStringUtils::Reset();
    }
};

/**
 * @brief 测试中文标点符号检测
 */
TEST_F(StringUtilsAdvancedTest, ChinesePunctuationDetection) {
    auto& utils = StringUtils::Instance();
    
    // 测试包含中文标点的字符串
    EXPECT_TRUE(utils.HasChinesePunctuation("你好，世界"));  // 中文逗号
    EXPECT_TRUE(utils.HasChinesePunctuation("测试：数据"));  // 中文冒号
    EXPECT_TRUE(utils.HasChinesePunctuation("问题？答案"));  // 中文问号
    EXPECT_TRUE(utils.HasChinesePunctuation("感叹！号"));    // 中文感叹号
    EXPECT_TRUE(utils.HasChinesePunctuation("引用\u201c内容\u201d"));  // 中文引号
    EXPECT_TRUE(utils.HasChinesePunctuation("括号（内容）")); // 中文括号
    EXPECT_TRUE(utils.HasChinesePunctuation("分号；测试"));  // 中文分号
    
    // 测试不包含中文标点的字符串
    EXPECT_FALSE(utils.HasChinesePunctuation("hello, world"));  // 英文逗号
    EXPECT_FALSE(utils.HasChinesePunctuation("test: data"));    // 英文冒号
    EXPECT_FALSE(utils.HasChinesePunctuation("question?"));     // 英文问号
    EXPECT_FALSE(utils.HasChinesePunctuation("exclaim!"));      // 英文感叹号
    EXPECT_FALSE(utils.HasChinesePunctuation("quote \"text\"")); // 英文引号
    EXPECT_FALSE(utils.HasChinesePunctuation("paren (text)"));  // 英文括号
    
    // 测试混合情况
    EXPECT_TRUE(utils.HasChinesePunctuation("hello，world"));   // 混合中英文
    EXPECT_FALSE(utils.HasChinesePunctuation("你好world"));     // 中英文但无中文标点
    
    // 测试空字符串和边界情况
    EXPECT_FALSE(utils.HasChinesePunctuation(""));
    EXPECT_FALSE(utils.HasChinesePunctuation("abc123"));
    EXPECT_FALSE(utils.HasChinesePunctuation("你好世界"));      // 纯中文无标点
}

/**
 * @brief 测试标点符号标准化
 */
TEST_F(StringUtilsAdvancedTest, PunctuationNormalization) {
    auto& utils = StringUtils::Instance();
    
    // 基础中文标点转英文标点
    EXPECT_EQ(utils.NormalizePunctuation("你好，世界"), "你好,世界");
    EXPECT_EQ(utils.NormalizePunctuation("测试：数据"), "测试:数据");
    EXPECT_EQ(utils.NormalizePunctuation("问题？"), "问题?");
    EXPECT_EQ(utils.NormalizePunctuation("感叹！"), "感叹!");
    EXPECT_EQ(utils.NormalizePunctuation("引用\u201c内容\u201d"), "引用\"内容\"");
    EXPECT_EQ(utils.NormalizePunctuation("括号（内容）"), "括号(内容)");
    EXPECT_EQ(utils.NormalizePunctuation("分号；测试"), "分号;测试");
    
    // 多个标点符号
    EXPECT_EQ(utils.NormalizePunctuation("你好，世界！"), "你好,世界!");
    EXPECT_EQ(utils.NormalizePunctuation("问题：答案？"), "问题:答案?");
    
    // 已经是英文标点的不应该改变
    EXPECT_EQ(utils.NormalizePunctuation("hello, world!"), "hello, world!");
    EXPECT_EQ(utils.NormalizePunctuation("test: data?"), "test: data?");
    
    // 混合标点
    EXPECT_EQ(utils.NormalizePunctuation("hello，world!"), "hello,world!");
    
    // 空字符串和无标点字符串
    EXPECT_EQ(utils.NormalizePunctuation(""), "");
    EXPECT_EQ(utils.NormalizePunctuation("hello world"), "hello world");
    EXPECT_EQ(utils.NormalizePunctuation("你好世界"), "你好世界");
}

/**
 * @brief 测试输入法兼容性处理集成
 */
TEST_F(StringUtilsAdvancedTest, InputMethodCompatibility) {
    auto& utils = StringUtils::Instance();
    
    // 测试包含中文标点的字符串解析
    std::string input_with_chinese_punct = "数据1，数据2，数据3";
    
    // 直接解析（会包含中文逗号）
    auto direct_result = utils.Split(input_with_chinese_punct, "，");
    std::vector<std::string> expected_direct = {"数据1", "数据2", "数据3"};
    EXPECT_EQ(direct_result, expected_direct);
    
    // 标准化后解析
    auto normalized = utils.NormalizePunctuation(input_with_chinese_punct);
    auto normalized_result = utils.Split(normalized, ",");
    std::vector<std::string> expected_normalized = {"数据1", "数据2", "数据3"};
    EXPECT_EQ(normalized_result, expected_normalized);
    
    // 测试键值对的处理
    std::string kv_with_chinese = "姓名：张三，年龄：25，城市：北京";
    auto normalized_kv = utils.NormalizePunctuation(kv_with_chinese);
    auto kv_result = utils.ParseToMap<std::string, std::string>(normalized_kv, ",", ":");
    
    std::map<std::string, std::string> expected_kv = {
        {"姓名", "张三"},
        {"年龄", "25"},
        {"城市", "北京"}
    };
    EXPECT_EQ(kv_result, expected_kv);
}

/**
 * @brief 测试ThreadSafeStringUtils基础功能
 */
TEST_F(StringUtilsAdvancedTest, ThreadSafeStringUtilsBasic) {
    auto& thread_safe_utils = ThreadSafeStringUtils::Instance();
    
    // 基础功能测试
    auto split_result = thread_safe_utils.Split("a-b-c");
    std::vector<std::string> expected = {"a", "b", "c"};
    EXPECT_EQ(split_result, expected);
    
    auto join_result = thread_safe_utils.Join(expected, "|");
    EXPECT_EQ(join_result, "a|b|c");
    
    // 解析功能测试
    std::vector<int> parse_result;
    thread_safe_utils.ParseToVector("1-2-3", parse_result);
    std::vector<int> expected_int = {1, 2, 3};
    EXPECT_EQ(parse_result, expected_int);
    
    auto vector_result = thread_safe_utils.ParseToVector<int>("10-20-30");
    std::vector<int> expected_vector = {10, 20, 30};
    EXPECT_EQ(vector_result, expected_vector);
    
    // Map解析测试
    std::map<std::string, int> map_result;
    thread_safe_utils.ParseToMap("key1:1,key2:2", map_result);
    std::map<std::string, int> expected_map = {{"key1", 1}, {"key2", 2}};
    EXPECT_EQ(map_result, expected_map);
    
    // 时间处理测试
    auto now = std::chrono::system_clock::now();
    auto time_string = thread_safe_utils.TimeToString(now);
    auto time_back = thread_safe_utils.StringToTime(time_string);
    
    // 允许小的时间差
    auto diff = now > time_back ? now - time_back : time_back - now;
    EXPECT_LT(diff.count(), std::chrono::seconds(2).count());
}

/**
 * @brief 测试ThreadSafeStringUtils的线程安全性
 */
TEST_F(StringUtilsAdvancedTest, ThreadSafeStringUtilsMultiThreading) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    std::vector<std::vector<std::string>> results(num_threads);
    
    // 多线程同时执行字符串操作
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            auto& utils = ThreadSafeStringUtils::Instance();
            
            for (int j = 0; j < operations_per_thread; ++j) {
                // 执行各种字符串操作
                std::string test_data = "data" + std::to_string(i) + "-" + std::to_string(j);
                auto split_result = utils.Split(test_data);
                results[i].insert(results[i].end(), split_result.begin(), split_result.end());
                
                auto joined = utils.Join(split_result, "|");
                (void)joined; // 防止编译器优化
                
                total_operations.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证所有操作都成功完成
    EXPECT_EQ(total_operations.load(), num_threads * operations_per_thread);
    
    // 验证每个线程的结果
    for (int i = 0; i < num_threads; ++i) {
        EXPECT_EQ(results[i].size(), operations_per_thread * 2); // 每次操作产生2个元素
        for (int j = 0; j < operations_per_thread; ++j) {
            std::string expected1 = "data" + std::to_string(i);
            std::string expected2 = std::to_string(j);
            EXPECT_EQ(results[i][j * 2], expected1);
            EXPECT_EQ(results[i][j * 2 + 1], expected2);
        }
    }
}

/**
 * @brief 测试并发解析操作
 */
TEST_F(StringUtilsAdvancedTest, ConcurrentParsingOperations) {
    const int num_threads = 20;
    const int iterations = 50;
    
    std::vector<std::future<bool>> futures;
    
    // 启动多个异步任务进行解析操作
    for (int i = 0; i < num_threads; ++i) {
        futures.push_back(std::async(std::launch::async, [i, iterations]() {
            auto& utils = ThreadSafeStringUtils::Instance();
            
            for (int j = 0; j < iterations; ++j) {
                // 测试vector解析
                auto vector_result = utils.ParseToVector<int>("1-2-3-4-5");
                if (vector_result.size() != 5 || vector_result[0] != 1 || vector_result[4] != 5) {
                    return false;
                }
                
                // 测试map解析
                std::map<std::string, int> map_result;
                utils.ParseToMap("a:1,b:2,c:3", map_result);
                if (map_result.size() != 3 || map_result["a"] != 1 || map_result["c"] != 3) {
                    return false;
                }
                
                // 测试时间转换
                auto now = std::chrono::system_clock::now();
                auto time_str = utils.TimeToString(now);
                if (time_str.empty()) {
                    return false;
                }
            }
            
            return true;
        }));
    }
    
    // 等待所有任务完成并验证结果
    for (auto& future : futures) {
        EXPECT_TRUE(future.get());
    }
}

/**
 * @brief 测试性能对比（线程安全 vs 非线程安全）
 */
TEST_F(StringUtilsAdvancedTest, PerformanceComparison) {
    const int iterations = 1000;
    
    // 测试非线程安全版本
    auto start = std::chrono::high_resolution_clock::now();
    {
        auto& utils = StringUtils::Instance();
        for (int i = 0; i < iterations; ++i) {
            auto result = utils.Split("a-b-c-d-e");
            auto joined = utils.Join(result, "|");
            auto parsed = utils.ParseToVector<int>("1-2-3-4-5");
            (void)joined; (void)parsed; // 防止编译器优化
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto unsafe_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 测试线程安全版本
    start = std::chrono::high_resolution_clock::now();
    {
        auto& utils = ThreadSafeStringUtils::Instance();
        for (int i = 0; i < iterations; ++i) {
            auto result = utils.Split("a-b-c-d-e");
            auto joined = utils.Join(result, "|");
            auto parsed = utils.ParseToVector<int>("1-2-3-4-5");
            (void)joined; (void)parsed; // 防止编译器优化
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto safe_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "非线程安全版本 (" << iterations << " 次): " 
              << unsafe_duration.count() << " 微秒" << std::endl;
    std::cout << "线程安全版本 (" << iterations << " 次): " 
              << safe_duration.count() << " 微秒" << std::endl;
    
    // 性能应该在合理范围内
    EXPECT_GT(unsafe_duration.count(), 0);
    EXPECT_GT(safe_duration.count(), 0);
    
    // 线程安全版本可能稍慢，但不应该有数量级的差异
    EXPECT_LT(safe_duration.count(), unsafe_duration.count() * 10);
}

/**
 * @brief 测试内存安全和资源管理
 */
TEST_F(StringUtilsAdvancedTest, MemorySafetyAndResourceManagement) {
    // 大量字符串操作测试内存泄漏
    const int large_iterations = 10000;
    
    {
        auto& utils = StringUtils::Instance();
        
        for (int i = 0; i < large_iterations; ++i) {
            std::string large_data;
            for (int j = 0; j < 100; ++j) {
                if (j > 0) large_data += "-";
                large_data += std::to_string(j);
            }
            
            auto result = utils.Split(large_data);
            auto rejoined = utils.Join(result);
            auto parsed = utils.ParseToVector<int>(large_data);
            
            // 验证结果正确性
            EXPECT_EQ(result.size(), 100);
            EXPECT_EQ(parsed.size(), 100);
            
            // 每100次迭代重置一次单例以测试资源清理
            if (i % 100 == 99) {
                StringUtils::Reset();
            }
        }
    }
    
    // 测试异常安全
    auto& utils = StringUtils::Instance();
    for (int i = 0; i < 1000; ++i) {
        std::vector<int> result;
        bool success = utils.TryParseToVector("1-2-invalid-4", result);
        EXPECT_FALSE(success);
        EXPECT_TRUE(result.empty()); // 异常情况下应该清空结果
    }
}

/**
 * @brief 测试极限条件和边界情况
 */
TEST_F(StringUtilsAdvancedTest, ExtremeCasesAndBoundaries) {
    auto& utils = StringUtils::Instance();
    
    // 极长字符串处理
    std::string very_long_string;
    for (int i = 0; i < 10000; ++i) {
        if (i > 0) very_long_string += "-";
        very_long_string += "item" + std::to_string(i);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = utils.Split(very_long_string);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(result.size(), 10000);
    EXPECT_EQ(result[0], "item0");
    EXPECT_EQ(result[9999], "item9999");
    EXPECT_LT(duration.count(), 1000); // 应该在1秒内完成
    
    // 极长分隔符
    std::string long_delimiter(1000, 'x');
    auto long_del_result = utils.Split("a" + long_delimiter + "b" + long_delimiter + "c", long_delimiter);
    std::vector<std::string> expected_long_del = {"a", "b", "c"};
    EXPECT_EQ(long_del_result, expected_long_del);
    
    // 空元素处理
    auto empty_elements = utils.Split("a---b---c", "-", false);
    std::vector<std::string> expected_empty = {"a", "", "", "b", "", "", "c"};
    EXPECT_EQ(empty_elements, expected_empty);
    
    // Unicode字符串复杂情况
    std::string unicode_complex = "中文1，English2，数字3，符号#4";
    auto normalized_unicode = utils.NormalizePunctuation(unicode_complex);
    auto unicode_result = utils.Split(normalized_unicode, ",");
    EXPECT_EQ(unicode_result.size(), 4);
    EXPECT_TRUE(utils.HasChinesePunctuation(unicode_complex));
    EXPECT_FALSE(utils.HasChinesePunctuation(normalized_unicode));
}

/**
 * @brief 测试集成场景
 */
TEST_F(StringUtilsAdvancedTest, IntegrationScenarios) {
    auto& utils = StringUtils::Instance();
    
    // 场景1：配置文件解析
    std::string config_data = "数据库主机：localhost，端口：3306，用户名：admin，密码：secret123";
    auto normalized_config = utils.NormalizePunctuation(config_data);
    auto config_map = utils.ParseToMap<std::string, std::string>(normalized_config, ",", ":");
    
    EXPECT_EQ(config_map["数据库主机"], "localhost");
    EXPECT_EQ(config_map["端口"], "3306");
    EXPECT_EQ(config_map["用户名"], "admin");
    EXPECT_EQ(config_map["密码"], "secret123");
    
    // 场景2：CSV数据处理
    std::vector<std::string> csv_lines = {
        "姓名，年龄，城市",
        "张三，25，北京",
        "李四，30，上海",
        "王五，28，广州"
    };
    
    std::vector<std::vector<std::string>> csv_data;
    for (const auto& line : csv_lines) {
        auto normalized_line = utils.NormalizePunctuation(line);
        auto fields = utils.Split(normalized_line, ",");
        csv_data.push_back(fields);
    }
    
    EXPECT_EQ(csv_data.size(), 4);
    EXPECT_EQ(csv_data[0], std::vector<std::string>({"姓名", "年龄", "城市"}));
    EXPECT_EQ(csv_data[1], std::vector<std::string>({"张三", "25", "北京"}));
    
    // 场景3：日志时间戳处理
    auto now = std::chrono::system_clock::now();
    auto log_timestamp = utils.TimeToString(now, "[%Y-%m-%d %H:%M:%S]");
    auto log_line = log_timestamp + " INFO: 系统启动完成";
    
    // 提取时间戳
    if (log_line.front() == '[') {
        auto end_bracket = log_line.find(']');
        if (end_bracket != std::string::npos) {
            auto timestamp_str = log_line.substr(1, end_bracket - 1);
            auto parsed_time = utils.StringToTime(timestamp_str, "%Y-%m-%d %H:%M:%S");
            
            // 验证时间解析正确（允许小误差）
            auto diff = now > parsed_time ? now - parsed_time : parsed_time - now;
            EXPECT_LT(diff.count(), std::chrono::seconds(2).count());
        }
    }
}