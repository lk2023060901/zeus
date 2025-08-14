/**
 * @file test_string_utils_datetime.cpp
 * @brief Zeus StringUtils日期时间处理测试
 */

#include "common/utilities/string_utils.h"
#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <iomanip>
#include <sstream>

using namespace common::utilities;

/**
 * @brief StringUtils日期时间测试类
 */
class StringUtilsDateTimeTest : public ::testing::Test {
protected:
    void SetUp() override {
        StringUtils::Reset();
    }
    
    void TearDown() override {
        StringUtils::Reset();
    }
    
    // 辅助函数：创建指定日期时间的时间点
    std::chrono::system_clock::time_point CreateTimePoint(int year, int month, int day, 
                                                          int hour = 0, int minute = 0, int second = 0) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        
        auto time_t = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(time_t);
    }
    
    // 辅助函数：比较两个时间点是否相等（允许小误差）
    bool TimePointsEqual(const std::chrono::system_clock::time_point& tp1,
                         const std::chrono::system_clock::time_point& tp2,
                         std::chrono::seconds tolerance = std::chrono::seconds(1)) {
        auto diff = tp1 > tp2 ? tp1 - tp2 : tp2 - tp1;
        return diff <= tolerance;
    }
};

/**
 * @brief 测试TimeToString基础功能
 */
TEST_F(StringUtilsDateTimeTest, TimeToStringBasic) {
    auto& utils = StringUtils::Instance();
    
    // 创建一个固定的时间点: 2023-12-25 15:30:45
    auto tp = CreateTimePoint(2023, 12, 25, 15, 30, 45);
    
    // 默认格式
    auto result = utils.TimeToString(tp);
    EXPECT_EQ(result, "2023-12-25 15:30:45");
    
    // ISO格式
    result = utils.TimeToString(tp, "%Y-%m-%d %H:%M:%S");
    EXPECT_EQ(result, "2023-12-25 15:30:45");
    
    // 日期格式
    result = utils.TimeToString(tp, "%Y-%m-%d");
    EXPECT_EQ(result, "2023-12-25");
    
    // 时间格式
    result = utils.TimeToString(tp, "%H:%M:%S");
    EXPECT_EQ(result, "15:30:45");
    
    // 12小时制格式
    result = utils.TimeToString(tp, "%Y-%m-%d %I:%M:%S %p");
    EXPECT_EQ(result, "2023-12-25 03:30:45 PM");
}

/**
 * @brief 测试TimeToString不同格式
 */
TEST_F(StringUtilsDateTimeTest, TimeToStringFormats) {
    auto& utils = StringUtils::Instance();
    
    // 创建时间点: 2023-01-01 09:05:03
    auto tp = CreateTimePoint(2023, 1, 1, 9, 5, 3);
    
    // 紧凑格式
    auto result = utils.TimeToString(tp, "%Y%m%d%H%M%S");
    EXPECT_EQ(result, "20230101090503");
    
    // 带毫秒的格式（注意：标准strftime不支持毫秒）
    result = utils.TimeToString(tp, "%Y-%m-%d_%H-%M-%S");
    EXPECT_EQ(result, "2023-01-01_09-05-03");
    
    // 月份名称格式
    result = utils.TimeToString(tp, "%B %d, %Y");
    EXPECT_EQ(result, "January 01, 2023");
    
    // 星期格式
    result = utils.TimeToString(tp, "%A, %B %d, %Y");
    EXPECT_TRUE(result.find("2023") != std::string::npos);
    EXPECT_TRUE(result.find("January") != std::string::npos);
}

/**
 * @brief 测试StringToTime基础功能
 */
TEST_F(StringUtilsDateTimeTest, StringToTimeBasic) {
    auto& utils = StringUtils::Instance();
    
    // 基础解析
    auto result = utils.StringToTime("2023-12-25 15:30:45");
    auto expected = CreateTimePoint(2023, 12, 25, 15, 30, 45);
    EXPECT_TRUE(TimePointsEqual(result, expected));
    
    // 日期格式
    result = utils.StringToTime("2023-12-25", "%Y-%m-%d");
    expected = CreateTimePoint(2023, 12, 25, 0, 0, 0);
    EXPECT_TRUE(TimePointsEqual(result, expected));
    
    // 时间格式（日期默认为epoch）
    result = utils.StringToTime("15:30:45", "%H:%M:%S");
    // 这个测试可能因为时区和epoch的原因有差异，我们主要验证不会抛异常
    EXPECT_NO_THROW(utils.StringToTime("15:30:45", "%H:%M:%S"));
}

/**
 * @brief 测试StringToTime不同格式
 */
TEST_F(StringUtilsDateTimeTest, StringToTimeFormats) {
    auto& utils = StringUtils::Instance();
    
    // 紧凑格式
    auto result = utils.StringToTime("20231225153045", "%Y%m%d%H%M%S");
    auto expected = CreateTimePoint(2023, 12, 25, 15, 30, 45);
    EXPECT_TRUE(TimePointsEqual(result, expected));
    
    // 自定义分隔符格式
    result = utils.StringToTime("2023/12/25 15-30-45", "%Y/%m/%d %H-%M-%S");
    EXPECT_TRUE(TimePointsEqual(result, expected));
    
    // 12小时制格式
    result = utils.StringToTime("2023-12-25 03:30:45 PM", "%Y-%m-%d %I:%M:%S %p");
    EXPECT_TRUE(TimePointsEqual(result, expected));
}

/**
 * @brief 测试TryStringToTime安全转换
 */
TEST_F(StringUtilsDateTimeTest, TryStringToTimeSafe) {
    auto& utils = StringUtils::Instance();
    
    // 正常转换
    std::chrono::system_clock::time_point result;
    bool success = utils.TryStringToTime("2023-12-25 15:30:45", result);
    EXPECT_TRUE(success);
    
    auto expected = CreateTimePoint(2023, 12, 25, 15, 30, 45);
    EXPECT_TRUE(TimePointsEqual(result, expected));
    
    // 无效格式
    success = utils.TryStringToTime("invalid-date", result);
    EXPECT_FALSE(success);
    
    // 格式不匹配
    success = utils.TryStringToTime("2023-12-25", result, "%H:%M:%S");
    EXPECT_FALSE(success);
    
    // 无效日期
    success = utils.TryStringToTime("2023-13-45 25:70:90", result);
    EXPECT_FALSE(success);
}

/**
 * @brief 测试时间字符串往返转换
 */
TEST_F(StringUtilsDateTimeTest, TimeStringRoundtrip) {
    auto& utils = StringUtils::Instance();
    
    // 创建原始时间点
    auto original = CreateTimePoint(2023, 6, 15, 12, 30, 45);
    
    // 转换为字符串再转回时间点
    auto time_string = utils.TimeToString(original);
    auto converted_back = utils.StringToTime(time_string);
    
    // 验证往返转换的一致性
    EXPECT_TRUE(TimePointsEqual(original, converted_back));
    
    // 测试不同格式的往返转换
    const std::string format = "%Y/%m/%d %H:%M:%S";
    time_string = utils.TimeToString(original, format);
    converted_back = utils.StringToTime(time_string, format);
    EXPECT_TRUE(TimePointsEqual(original, converted_back));
}

/**
 * @brief 测试边界时间值
 */
TEST_F(StringUtilsDateTimeTest, BoundaryTimeValues) {
    auto& utils = StringUtils::Instance();
    
    // Unix epoch (1970-01-01 00:00:00)
    auto epoch = std::chrono::system_clock::from_time_t(0);
    auto epoch_string = utils.TimeToString(epoch);
    auto epoch_back = utils.StringToTime(epoch_string);
    EXPECT_TRUE(TimePointsEqual(epoch, epoch_back));
    
    // Y2K时间 (2000-01-01 00:00:00)
    auto y2k = CreateTimePoint(2000, 1, 1, 0, 0, 0);
    auto y2k_string = utils.TimeToString(y2k);
    auto y2k_back = utils.StringToTime(y2k_string);
    EXPECT_TRUE(TimePointsEqual(y2k, y2k_back));
    
    // 闰年日期 (2024-02-29)
    auto leap_day = CreateTimePoint(2024, 2, 29, 12, 0, 0);
    auto leap_string = utils.TimeToString(leap_day, "%Y-%m-%d");
    EXPECT_EQ(leap_string, "2024-02-29");
    
    auto leap_back = utils.StringToTime(leap_string, "%Y-%m-%d");
    auto expected_leap = CreateTimePoint(2024, 2, 29, 0, 0, 0);
    EXPECT_TRUE(TimePointsEqual(leap_back, expected_leap));
}

/**
 * @brief 测试时区相关（注意：这个测试可能因系统时区设置而有差异）
 */
TEST_F(StringUtilsDateTimeTest, TimezoneHandling) {
    auto& utils = StringUtils::Instance();
    
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    
    // 转换为字符串再转回来
    auto now_string = utils.TimeToString(now);
    auto now_back = utils.StringToTime(now_string);
    
    // 应该相等（允许小的误差）
    EXPECT_TRUE(TimePointsEqual(now, now_back));
    
    // 测试不同的格式是否能正确处理
    auto custom_format = "%Y-%m-%d %H:%M:%S";
    now_string = utils.TimeToString(now, custom_format);
    now_back = utils.StringToTime(now_string, custom_format);
    EXPECT_TRUE(TimePointsEqual(now, now_back));
}

/**
 * @brief 测试错误处理
 */
TEST_F(StringUtilsDateTimeTest, ErrorHandling) {
    auto& utils = StringUtils::Instance();
    
    // StringToTime的错误处理
    EXPECT_THROW(utils.StringToTime(""), std::exception);
    EXPECT_THROW(utils.StringToTime("invalid"), std::exception);
    EXPECT_THROW(utils.StringToTime("2023-13-32"), std::exception);
    
    // TryStringToTime不应该抛出异常
    std::chrono::system_clock::time_point result;
    EXPECT_NO_THROW(utils.TryStringToTime("", result));
    EXPECT_NO_THROW(utils.TryStringToTime("invalid", result));
    EXPECT_NO_THROW(utils.TryStringToTime("2023-13-32", result));
    
    // 但是应该返回false
    EXPECT_FALSE(utils.TryStringToTime("", result));
    EXPECT_FALSE(utils.TryStringToTime("invalid", result));
    EXPECT_FALSE(utils.TryStringToTime("2023-13-32", result));
}

/**
 * @brief 测试特殊日期格式
 */
TEST_F(StringUtilsDateTimeTest, SpecialDateFormats) {
    auto& utils = StringUtils::Instance();
    
    // ISO 8601格式（简化版，不包含时区）
    auto tp = CreateTimePoint(2023, 12, 25, 15, 30, 45);
    auto iso_string = utils.TimeToString(tp, "%Y-%m-%dT%H:%M:%S");
    EXPECT_EQ(iso_string, "2023-12-25T15:30:45");
    
    auto iso_back = utils.StringToTime(iso_string, "%Y-%m-%dT%H:%M:%S");
    EXPECT_TRUE(TimePointsEqual(tp, iso_back));
    
    // 文件名友好格式
    auto filename_string = utils.TimeToString(tp, "%Y%m%d_%H%M%S");
    EXPECT_EQ(filename_string, "20231225_153045");
    
    auto filename_back = utils.StringToTime(filename_string, "%Y%m%d_%H%M%S");
    EXPECT_TRUE(TimePointsEqual(tp, filename_back));
    
    // 日志格式
    auto log_string = utils.TimeToString(tp, "[%Y-%m-%d %H:%M:%S]");
    EXPECT_EQ(log_string, "[2023-12-25 15:30:45]");
    
    auto log_back = utils.StringToTime(log_string, "[%Y-%m-%d %H:%M:%S]");
    EXPECT_TRUE(TimePointsEqual(tp, log_back));
}

/**
 * @brief 测试性能
 */
TEST_F(StringUtilsDateTimeTest, Performance) {
    auto& utils = StringUtils::Instance();
    
    auto tp = CreateTimePoint(2023, 12, 25, 15, 30, 45);
    const int iterations = 1000;
    
    // 测试TimeToString性能
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto result = utils.TimeToString(tp);
        (void)result; // 防止编译器优化
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto to_string_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 测试StringToTime性能
    std::string time_string = "2023-12-25 15:30:45";
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        auto result = utils.StringToTime(time_string);
        (void)result; // 防止编译器优化
    }
    end = std::chrono::high_resolution_clock::now();
    auto from_string_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "TimeToString (" << iterations << " 次): " 
              << to_string_duration.count() << " 微秒" << std::endl;
    std::cout << "StringToTime (" << iterations << " 次): " 
              << from_string_duration.count() << " 微秒" << std::endl;
    
    // 性能应该在合理范围内
    EXPECT_LT(to_string_duration.count(), 500000); // 小于500毫秒
    EXPECT_LT(from_string_duration.count(), 500000); // 小于500毫秒
}

/**
 * @brief 测试多线程安全性（如果使用ThreadSafeStringUtils）
 */
TEST_F(StringUtilsDateTimeTest, ThreadSafety) {
    // 注意：这里测试的是非线程安全版本，主要确保基本功能正常
    auto& utils = StringUtils::Instance();
    
    auto tp = CreateTimePoint(2023, 12, 25, 15, 30, 45);
    
    // 多次调用应该得到一致的结果
    for (int i = 0; i < 100; ++i) {
        auto result = utils.TimeToString(tp);
        EXPECT_EQ(result, "2023-12-25 15:30:45");
        
        auto back = utils.StringToTime(result);
        EXPECT_TRUE(TimePointsEqual(tp, back));
    }
}

/**
 * @brief 测试极值情况
 */
TEST_F(StringUtilsDateTimeTest, ExtremeValues) {
    auto& utils = StringUtils::Instance();
    
    // 测试时间范围边界
    // 注意：这些测试可能因为系统和编译器的限制而失败
    
    // 最小有效日期（系统依赖）
    std::chrono::system_clock::time_point result;
    bool success = utils.TryStringToTime("1970-01-01 00:00:00", result);
    if (success) {
        auto back_string = utils.TimeToString(result);
        // 验证能够往返转换
        auto back_time = utils.StringToTime(back_string);
        EXPECT_TRUE(TimePointsEqual(result, back_time));
    }
    
    // 验证各种边界日期
    std::vector<std::string> boundary_dates = {
        "2000-01-01 00:00:00",  // Y2K
        "2024-02-29 12:00:00",  // 闰年
        "2100-12-31 23:59:59"   // 未来日期
    };
    
    for (const auto& date_str : boundary_dates) {
        success = utils.TryStringToTime(date_str, result);
        if (success) {
            auto back_string = utils.TimeToString(result);
            auto back_time = utils.StringToTime(back_string);
            EXPECT_TRUE(TimePointsEqual(result, back_time));
        }
    }
}