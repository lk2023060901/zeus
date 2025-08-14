/**
 * @file test_string_utils_basic.cpp
 * @brief Zeus StringUtils基础字符串操作测试
 */

#include "common/utilities/string_utils.h"
#include <gtest/gtest.h>
#include <vector>
#include <string>

using namespace common::utilities;

/**
 * @brief StringUtils基础功能测试类
 */
class StringUtilsBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 重置单例以确保测试独立性
        StringUtils::Reset();
    }
    
    void TearDown() override {
        StringUtils::Reset();
    }
};

/**
 * @brief 测试单例访问
 */
TEST_F(StringUtilsBasicTest, SingletonAccess) {
    // 测试GetInstance()
    auto& instance1 = StringUtils::GetInstance();
    auto& instance2 = StringUtils::GetInstance();
    
    // 验证是同一个实例
    EXPECT_EQ(&instance1, &instance2);
    
    // 测试Instance()访问器
    auto& instance3 = StringUtils::Instance();
    EXPECT_EQ(&instance1, &instance3);
    
    // 验证实例已创建
    EXPECT_TRUE(StringUtils::IsInstanceCreated());
}

/**
 * @brief 测试Split基础功能
 */
TEST_F(StringUtilsBasicTest, SplitBasic) {
    auto& utils = StringUtils::Instance();
    
    // 基础分割测试
    auto result = utils.Split("a-b-c");
    std::vector<std::string> expected = {"a", "b", "c"};
    EXPECT_EQ(result, expected);
    
    // 自定义分隔符
    result = utils.Split("a|b|c", "|");
    expected = {"a", "b", "c"};
    EXPECT_EQ(result, expected);
    
    // 多字符分隔符
    result = utils.Split("a::b::c", "::");
    expected = {"a", "b", "c"};
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试Split空字符串处理
 */
TEST_F(StringUtilsBasicTest, SplitEmptyStrings) {
    auto& utils = StringUtils::Instance();
    
    // 空输入字符串
    auto result = utils.Split("");
    EXPECT_TRUE(result.empty());
    
    // 包含空元素，默认跳过
    result = utils.Split("a--b", "-");
    std::vector<std::string> expected = {"a", "b"};
    EXPECT_EQ(result, expected);
    
    // 包含空元素，不跳过
    result = utils.Split("a--b", "-", false);
    expected = {"a", "", "b"};
    EXPECT_EQ(result, expected);
    
    // 开头和结尾有分隔符
    result = utils.Split("-a-b-", "-");
    expected = {"a", "b"};
    EXPECT_EQ(result, expected);
    
    result = utils.Split("-a-b-", "-", false);
    expected = {"", "a", "b", ""};
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试Split特殊情况
 */
TEST_F(StringUtilsBasicTest, SplitSpecialCases) {
    auto& utils = StringUtils::Instance();
    
    // 没有分隔符
    auto result = utils.Split("hello");
    std::vector<std::string> expected = {"hello"};
    EXPECT_EQ(result, expected);
    
    // 只有分隔符
    result = utils.Split("---", "-");
    EXPECT_TRUE(result.empty());
    
    result = utils.Split("---", "-", false);
    expected = {"", "", "", ""};
    EXPECT_EQ(result, expected);
    
    // 分隔符不存在
    result = utils.Split("abc", "|");
    expected = {"abc"};
    EXPECT_EQ(result, expected);
    
    // 分隔符比字符串长
    result = utils.Split("ab", "xyz");
    expected = {"ab"};
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试Join基础功能
 */
TEST_F(StringUtilsBasicTest, JoinBasic) {
    auto& utils = StringUtils::Instance();
    
    // 基础连接测试
    std::vector<std::string> parts = {"a", "b", "c"};
    auto result = utils.Join(parts);
    EXPECT_EQ(result, "a-b-c");
    
    // 自定义分隔符
    result = utils.Join(parts, "|");
    EXPECT_EQ(result, "a|b|c");
    
    // 多字符分隔符
    result = utils.Join(parts, "::");
    EXPECT_EQ(result, "a::b::c");
}

/**
 * @brief 测试Join特殊情况
 */
TEST_F(StringUtilsBasicTest, JoinSpecialCases) {
    auto& utils = StringUtils::Instance();
    
    // 空向量
    std::vector<std::string> empty_parts;
    auto result = utils.Join(empty_parts);
    EXPECT_EQ(result, "");
    
    // 单个元素
    std::vector<std::string> single_part = {"hello"};
    result = utils.Join(single_part);
    EXPECT_EQ(result, "hello");
    
    // 包含空字符串
    std::vector<std::string> parts_with_empty = {"a", "", "b"};
    result = utils.Join(parts_with_empty);
    EXPECT_EQ(result, "a--b");
    
    // 全是空字符串
    std::vector<std::string> all_empty = {"", "", ""};
    result = utils.Join(all_empty);
    EXPECT_EQ(result, "--");
}

/**
 * @brief 测试Split和Join的往返一致性
 */
TEST_F(StringUtilsBasicTest, SplitJoinRoundtrip) {
    auto& utils = StringUtils::Instance();
    
    std::string original = "hello-world-test";
    auto parts = utils.Split(original);
    auto rejoined = utils.Join(parts);
    EXPECT_EQ(original, rejoined);
    
    // 使用不同分隔符
    original = "a|b|c|d";
    parts = utils.Split(original, "|");
    rejoined = utils.Join(parts, "|");
    EXPECT_EQ(original, rejoined);
    
    // 包含空元素的情况
    original = "a::b::c";
    parts = utils.Split(original, "::", false);
    rejoined = utils.Join(parts, "::");
    EXPECT_EQ(original, rejoined);
}

/**
 * @brief 测试Trim基础功能
 */
TEST_F(StringUtilsBasicTest, TrimBasic) {
    auto& utils = StringUtils::Instance();
    
    // 基础去空格
    EXPECT_EQ(utils.Trim("  hello  "), "hello");
    EXPECT_EQ(utils.Trim("\thello\t"), "hello");
    EXPECT_EQ(utils.Trim("\nhello\n"), "hello");
    EXPECT_EQ(utils.Trim("\rhello\r"), "hello");
    
    // 混合空白字符
    EXPECT_EQ(utils.Trim(" \t\n\rhello \t\n\r"), "hello");
    
    // 中间的空白字符不会被去除
    EXPECT_EQ(utils.Trim("  hello world  "), "hello world");
}

/**
 * @brief 测试Trim特殊情况
 */
TEST_F(StringUtilsBasicTest, TrimSpecialCases) {
    auto& utils = StringUtils::Instance();
    
    // 空字符串
    EXPECT_EQ(utils.Trim(""), "");
    
    // 只有空白字符
    EXPECT_EQ(utils.Trim("   "), "");
    EXPECT_EQ(utils.Trim("\t\n\r"), "");
    
    // 没有空白字符
    EXPECT_EQ(utils.Trim("hello"), "hello");
    
    // 只有前导空白
    EXPECT_EQ(utils.Trim("  hello"), "hello");
    
    // 只有尾随空白
    EXPECT_EQ(utils.Trim("hello  "), "hello");
    
    // 自定义要去除的字符
    EXPECT_EQ(utils.Trim("###hello###", "#"), "hello");
    EXPECT_EQ(utils.Trim("abchelloabc", "abc"), "hello");
}

/**
 * @brief 测试DetectDelimiter功能
 */
TEST_F(StringUtilsBasicTest, DetectDelimiter) {
    auto& utils = StringUtils::Instance();
    
    // 检测常见分隔符
    EXPECT_EQ(utils.DetectDelimiter("a-b-c"), "-");
    EXPECT_EQ(utils.DetectDelimiter("a|b|c"), "|");
    EXPECT_EQ(utils.DetectDelimiter("a_b_c"), "_");
    EXPECT_EQ(utils.DetectDelimiter("a,b,c"), ",");
    EXPECT_EQ(utils.DetectDelimiter("a:b:c"), ":");
    EXPECT_EQ(utils.DetectDelimiter("a;b;c"), ";");
    
    // 多个分隔符时选择最常见的
    EXPECT_EQ(utils.DetectDelimiter("a-b-c|d"), "-");
    EXPECT_EQ(utils.DetectDelimiter("a|b|c|d-e"), "|");
    
    // 没有识别到分隔符时返回默认值
    EXPECT_EQ(utils.DetectDelimiter("hello"), DefaultDelimiters::PRIMARY);
    EXPECT_EQ(utils.DetectDelimiter(""), DefaultDelimiters::PRIMARY);
}

/**
 * @brief 测试默认分隔符常量
 */
TEST_F(StringUtilsBasicTest, DefaultDelimiters) {
    // 验证默认分隔符常量
    EXPECT_STREQ(DefaultDelimiters::PRIMARY, "-");
    EXPECT_STREQ(DefaultDelimiters::PIPE, "|");
    EXPECT_STREQ(DefaultDelimiters::UNDERSCORE, "_");
    EXPECT_STREQ(DefaultDelimiters::SLASH, "/");
    EXPECT_STREQ(DefaultDelimiters::STAR, "*");
    EXPECT_STREQ(DefaultDelimiters::PLUS, "+");
    EXPECT_STREQ(DefaultDelimiters::EQUAL, "=");
    EXPECT_STREQ(DefaultDelimiters::HASH, "#");
    EXPECT_STREQ(DefaultDelimiters::AT, "@");
    EXPECT_STREQ(DefaultDelimiters::TAB, "\t");
    EXPECT_STREQ(DefaultDelimiters::SPACE, " ");
    EXPECT_STREQ(DefaultDelimiters::NEWLINE, "\n");
    EXPECT_STREQ(DefaultDelimiters::KV_SEPARATOR, ":");
    EXPECT_STREQ(DefaultDelimiters::PAIR_SEPARATOR, ",");
    
    // 验证默认分隔符可以正常使用
    auto& utils = StringUtils::Instance();
    
    auto result = utils.Split("a-b-c", DefaultDelimiters::PRIMARY);
    std::vector<std::string> expected = {"a", "b", "c"};
    EXPECT_EQ(result, expected);
    
    result = utils.Split("a|b|c", DefaultDelimiters::PIPE);
    EXPECT_EQ(result, expected);
    
    result = utils.Split("a_b_c", DefaultDelimiters::UNDERSCORE);
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试性能相关的基础操作
 */
TEST_F(StringUtilsBasicTest, PerformanceBasics) {
    auto& utils = StringUtils::Instance();
    
    // 大字符串分割性能测试
    std::string large_string;
    for (int i = 0; i < 1000; ++i) {
        if (i > 0) large_string += "-";
        large_string += std::to_string(i);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = utils.Split(large_string);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 验证结果正确性
    EXPECT_EQ(result.size(), 1000);
    EXPECT_EQ(result[0], "0");
    EXPECT_EQ(result[999], "999");
    
    // 性能应该在合理范围内（具体时间取决于硬件）
    EXPECT_LT(duration.count(), 50000); // 小于50毫秒
    
    std::cout << "大字符串分割时间: " << duration.count() << " 微秒" << std::endl;
}

/**
 * @brief 测试边界条件
 */
TEST_F(StringUtilsBasicTest, EdgeCases) {
    auto& utils = StringUtils::Instance();
    
    // 极长的分隔符
    std::string long_delimiter(100, 'x');
    auto result = utils.Split("a" + long_delimiter + "b", long_delimiter);
    std::vector<std::string> expected = {"a", "b"};
    EXPECT_EQ(result, expected);
    
    // 分隔符与内容相同
    result = utils.Split("abc", "abc");
    EXPECT_TRUE(result.empty());
    
    result = utils.Split("abc", "abc", false);
    expected = {"", ""};
    EXPECT_EQ(result, expected);
    
    // Unicode字符串（UTF-8）
    result = utils.Split("你好-世界-测试");
    expected = {"你好", "世界", "测试"};
    EXPECT_EQ(result, expected);
    
    // 包含特殊字符
    result = utils.Split("a\0b\0c", std::string("\0", 1));
    expected = {"a", "b", "c"};
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试内存安全性
 */
TEST_F(StringUtilsBasicTest, MemorySafety) {
    auto& utils = StringUtils::Instance();
    
    // 大量小字符串操作
    for (int i = 0; i < 1000; ++i) {
        std::string test_str = "test" + std::to_string(i) + "-data";
        auto result = utils.Split(test_str);
        EXPECT_EQ(result.size(), 2);
        EXPECT_EQ(result[0], "test" + std::to_string(i));
        EXPECT_EQ(result[1], "data");
    }
    
    // 大量Join操作
    std::vector<std::string> parts = {"a", "b", "c", "d", "e"};
    for (int i = 0; i < 1000; ++i) {
        auto result = utils.Join(parts, std::to_string(i));
        EXPECT_TRUE(result.find("a") != std::string::npos);
        EXPECT_TRUE(result.find("e") != std::string::npos);
    }
}