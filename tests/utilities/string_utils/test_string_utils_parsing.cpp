/**
 * @file test_string_utils_parsing.cpp
 * @brief Zeus StringUtils类型安全解析测试
 */

#include "common/utilities/string_utils.h"
#include <gtest/gtest.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <cstdint>

using namespace common::utilities;

/**
 * @brief StringUtils解析功能测试类
 */
class StringUtilsParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        StringUtils::Reset();
    }
    
    void TearDown() override {
        StringUtils::Reset();
    }
};

/**
 * @brief 测试ParseToVector基础类型
 */
TEST_F(StringUtilsParsingTest, ParseToVectorBasicTypes) {
    auto& utils = StringUtils::Instance();
    
    // 测试int类型
    std::vector<int> int_result;
    utils.ParseToVector("1-2-3-4-5", int_result);
    std::vector<int> expected_int = {1, 2, 3, 4, 5};
    EXPECT_EQ(int_result, expected_int);
    
    // 测试int32_t类型
    std::vector<int32_t> int32_result;
    utils.ParseToVector("10-20-30", int32_result);
    std::vector<int32_t> expected_int32 = {10, 20, 30};
    EXPECT_EQ(int32_result, expected_int32);
    
    // 测试int64_t类型
    std::vector<int64_t> int64_result;
    utils.ParseToVector("1000000000-2000000000", int64_result);
    std::vector<int64_t> expected_int64 = {1000000000LL, 2000000000LL};
    EXPECT_EQ(int64_result, expected_int64);
    
    // 测试uint32_t类型
    std::vector<uint32_t> uint32_result;
    utils.ParseToVector("100-200-300", uint32_result);
    std::vector<uint32_t> expected_uint32 = {100U, 200U, 300U};
    EXPECT_EQ(uint32_result, expected_uint32);
    
    // 测试double类型
    std::vector<double> double_result;
    utils.ParseToVector("1.5-2.7-3.14", double_result);
    std::vector<double> expected_double = {1.5, 2.7, 3.14};
    EXPECT_EQ(double_result, expected_double);
    
    // 测试float类型
    std::vector<float> float_result;
    utils.ParseToVector("0.5-1.0-1.5", float_result);
    std::vector<float> expected_float = {0.5f, 1.0f, 1.5f};
    EXPECT_EQ(float_result, expected_float);
    
    // 测试string类型
    std::vector<std::string> string_result;
    utils.ParseToVector("hello-world-test", string_result);
    std::vector<std::string> expected_string = {"hello", "world", "test"};
    EXPECT_EQ(string_result, expected_string);
}

/**
 * @brief 测试ParseToVector返回值版本
 */
TEST_F(StringUtilsParsingTest, ParseToVectorReturnValue) {
    auto& utils = StringUtils::Instance();
    
    // 测试int类型
    auto int_result = utils.ParseToVector<int>("1-2-3");
    std::vector<int> expected_int = {1, 2, 3};
    EXPECT_EQ(int_result, expected_int);
    
    // 测试string类型
    auto string_result = utils.ParseToVector<std::string>("a-b-c");
    std::vector<std::string> expected_string = {"a", "b", "c"};
    EXPECT_EQ(string_result, expected_string);
    
    // 测试double类型
    auto double_result = utils.ParseToVector<double>("1.1-2.2-3.3");
    std::vector<double> expected_double = {1.1, 2.2, 3.3};
    EXPECT_EQ(double_result, expected_double);
}

/**
 * @brief 测试ParseToVector自定义分隔符
 */
TEST_F(StringUtilsParsingTest, ParseToVectorCustomDelimiter) {
    auto& utils = StringUtils::Instance();
    
    // 使用管道符
    auto result = utils.ParseToVector<int>("1|2|3|4", "|");
    std::vector<int> expected = {1, 2, 3, 4};
    EXPECT_EQ(result, expected);
    
    // 使用逗号
    result = utils.ParseToVector<int>("10,20,30", ",");
    expected = {10, 20, 30};
    EXPECT_EQ(result, expected);
    
    // 使用空格
    result = utils.ParseToVector<int>("100 200 300", " ");
    expected = {100, 200, 300};
    EXPECT_EQ(result, expected);
    
    // 使用多字符分隔符
    result = utils.ParseToVector<int>("1::2::3", "::");
    expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试ParseToMap基础功能
 */
TEST_F(StringUtilsParsingTest, ParseToMapBasic) {
    auto& utils = StringUtils::Instance();
    
    // 测试string到int的映射
    std::map<std::string, int> result;
    utils.ParseToMap("name:1,age:25,score:100", result);
    
    std::map<std::string, int> expected = {
        {"name", 1},
        {"age", 25},
        {"score", 100}
    };
    EXPECT_EQ(result, expected);
    
    // 测试int到string的映射
    std::map<int, std::string> int_string_result;
    utils.ParseToMap("1:apple,2:banana,3:orange", int_string_result);
    
    std::map<int, std::string> expected_int_string = {
        {1, "apple"},
        {2, "banana"},
        {3, "orange"}
    };
    EXPECT_EQ(int_string_result, expected_int_string);
}

/**
 * @brief 测试ParseToMap返回值版本
 */
TEST_F(StringUtilsParsingTest, ParseToMapReturnValue) {
    auto& utils = StringUtils::Instance();
    
    auto result = utils.ParseToMap<std::string, int>("key1:10,key2:20,key3:30");
    
    std::map<std::string, int> expected = {
        {"key1", 10},
        {"key2", 20},
        {"key3", 30}
    };
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试ParseToMap自定义分隔符
 */
TEST_F(StringUtilsParsingTest, ParseToMapCustomDelimiters) {
    auto& utils = StringUtils::Instance();
    
    // 使用不同的键值对分隔符和键值分隔符
    auto result = utils.ParseToMap<std::string, int>("key1=10|key2=20|key3=30", "|", "=");
    
    std::map<std::string, int> expected = {
        {"key1", 10},
        {"key2", 20},
        {"key3", 30}
    };
    EXPECT_EQ(result, expected);
    
    // 使用分号和冒号
    result = utils.ParseToMap<std::string, int>("a:1;b:2;c:3", ";", ":");
    expected = {
        {"a", 1},
        {"b", 2},
        {"c", 3}
    };
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试ParseToUnorderedMap
 */
TEST_F(StringUtilsParsingTest, ParseToUnorderedMap) {
    auto& utils = StringUtils::Instance();
    
    std::unordered_map<std::string, int> result;
    utils.ParseToUnorderedMap("key1:100,key2:200,key3:300", result);
    
    // 因为unordered_map的顺序不确定，我们单独检查每个键值对
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result["key1"], 100);
    EXPECT_EQ(result["key2"], 200);
    EXPECT_EQ(result["key3"], 300);
}

/**
 * @brief 测试智能容器解析Parse方法
 */
TEST_F(StringUtilsParsingTest, SmartContainerParsing) {
    auto& utils = StringUtils::Instance();
    
    // 测试vector的智能解析
    std::vector<int> int_vector;
    int_vector = utils.Parse<std::vector<int>>("1-2-3-4");
    std::vector<int> expected_vector = {1, 2, 3, 4};
    EXPECT_EQ(int_vector, expected_vector);
    
    std::vector<std::string> string_vector;
    string_vector = utils.Parse<std::vector<std::string>>("hello-world-test");
    std::vector<std::string> expected_string_vector = {"hello", "world", "test"};
    EXPECT_EQ(string_vector, expected_string_vector);
    
    // 注意：Map类型需要使用显式的ParseToMap调用
    // utils.Parse<std::map<string, int>>(...) 会触发static_assert
}

/**
 * @brief 测试TryParseToVector安全解析
 */
TEST_F(StringUtilsParsingTest, TryParseToVectorSafe) {
    auto& utils = StringUtils::Instance();
    
    // 正常解析
    std::vector<int> result;
    bool success = utils.TryParseToVector("1-2-3", result);
    EXPECT_TRUE(success);
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
    
    // 解析失败的情况
    result.clear();
    success = utils.TryParseToVector("1-abc-3", result);
    EXPECT_FALSE(success);
    EXPECT_TRUE(result.empty()); // 失败时应该清空结果
    
    // 部分解析失败
    result.clear();
    success = utils.TryParseToVector("1-2-abc-4", result);
    EXPECT_FALSE(success);
    EXPECT_TRUE(result.empty());
}

/**
 * @brief 测试ParseToVectorSafe带默认值的安全解析
 */
TEST_F(StringUtilsParsingTest, ParseToVectorSafeWithDefault) {
    auto& utils = StringUtils::Instance();
    
    // 正常解析
    auto result = utils.ParseToVectorSafe<int>("1-2-3");
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(result, expected);
    
    // 解析失败时返回默认值（空向量）
    result = utils.ParseToVectorSafe<int>("1-abc-3");
    EXPECT_TRUE(result.empty());
    
    // 使用自定义默认值
    std::vector<int> default_value = {0, 0, 0};
    result = utils.ParseToVectorSafe<int>("invalid-data", default_value);
    EXPECT_EQ(result, default_value);
    
    // 正常解析时不使用默认值
    result = utils.ParseToVectorSafe<int>("10-20-30", default_value);
    expected = {10, 20, 30};
    EXPECT_EQ(result, expected);
}

/**
 * @brief 测试BatchParseToVector批量解析
 */
TEST_F(StringUtilsParsingTest, BatchParseToVector) {
    auto& utils = StringUtils::Instance();
    
    std::vector<std::string> input_strings = {
        "1-2-3",
        "10-20-30",
        "100-200-300"
    };
    
    auto results = utils.BatchParseToVector<int>(input_strings);
    
    EXPECT_EQ(results.size(), 3);
    
    std::vector<int> expected1 = {1, 2, 3};
    std::vector<int> expected2 = {10, 20, 30};
    std::vector<int> expected3 = {100, 200, 300};
    
    EXPECT_EQ(results[0], expected1);
    EXPECT_EQ(results[1], expected2);
    EXPECT_EQ(results[2], expected3);
}

/**
 * @brief 测试解析边界条件
 */
TEST_F(StringUtilsParsingTest, ParsingEdgeCases) {
    auto& utils = StringUtils::Instance();
    
    // 空字符串解析
    auto empty_result = utils.ParseToVector<int>("");
    EXPECT_TRUE(empty_result.empty());
    
    // 单个元素
    auto single_result = utils.ParseToVector<int>("42");
    std::vector<int> expected_single = {42};
    EXPECT_EQ(single_result, expected_single);
    
    // 负数解析
    auto negative_result = utils.ParseToVector<int>("-1--2--3");
    std::vector<int> expected_negative = {-1, -2, -3};
    EXPECT_EQ(negative_result, expected_negative);
    
    // 浮点数解析
    auto float_result = utils.ParseToVector<double>("3.14-2.71-1.41");
    std::vector<double> expected_float = {3.14, 2.71, 1.41};
    EXPECT_EQ(float_result, expected_float);
    
    // 科学记数法
    auto scientific_result = utils.ParseToVector<double>("1e3-2.5e-2-1.23e+5");
    std::vector<double> expected_scientific = {1000.0, 0.025, 123000.0};
    EXPECT_EQ(scientific_result, expected_scientific);
}

/**
 * @brief 测试Map解析边界条件
 */
TEST_F(StringUtilsParsingTest, MapParsingEdgeCases) {
    auto& utils = StringUtils::Instance();
    
    // 空字符串
    auto empty_map = utils.ParseToMap<std::string, int>("");
    EXPECT_TRUE(empty_map.empty());
    
    // 单个键值对
    auto single_map = utils.ParseToMap<std::string, int>("key:42");
    std::map<std::string, int> expected_single = {{"key", 42}};
    EXPECT_EQ(single_map, expected_single);
    
    // 缺少值的键值对（应该被跳过）
    auto incomplete_map = utils.ParseToMap<std::string, int>("key1:10,incomplete,key2:20");
    std::map<std::string, int> expected_incomplete = {
        {"key1", 10},
        {"key2", 20}
    };
    EXPECT_EQ(incomplete_map, expected_incomplete);
    
    // 重复键（后面的值会覆盖前面的）
    auto duplicate_map = utils.ParseToMap<std::string, int>("key:10,key:20");
    std::map<std::string, int> expected_duplicate = {{"key", 20}};
    EXPECT_EQ(duplicate_map, expected_duplicate);
}

/**
 * @brief 测试特殊字符处理
 */
TEST_F(StringUtilsParsingTest, SpecialCharacterHandling) {
    auto& utils = StringUtils::Instance();
    
    // 包含空格的字符串
    auto space_result = utils.ParseToVector<std::string>("hello world-test case");
    std::vector<std::string> expected_space = {"hello world", "test case"};
    EXPECT_EQ(space_result, expected_space);
    
    // 包含特殊字符的Map键值
    auto special_map = utils.ParseToMap<std::string, std::string>("key@#:value$%,test!:data*");
    std::map<std::string, std::string> expected_special = {
        {"key@#", "value$%"},
        {"test!", "data*"}
    };
    EXPECT_EQ(special_map, expected_special);
    
    // Unicode字符串解析
    auto unicode_result = utils.ParseToVector<std::string>("你好-世界-测试");
    std::vector<std::string> expected_unicode = {"你好", "世界", "测试"};
    EXPECT_EQ(unicode_result, expected_unicode);
}

/**
 * @brief 测试性能相关的解析操作
 */
TEST_F(StringUtilsParsingTest, ParsingPerformance) {
    auto& utils = StringUtils::Instance();
    
    // 大型向量解析
    std::string large_string;
    for (int i = 0; i < 1000; ++i) {
        if (i > 0) large_string += "-";
        large_string += std::to_string(i);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = utils.ParseToVector<int>(large_string);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 验证结果
    EXPECT_EQ(result.size(), 1000);
    EXPECT_EQ(result[0], 0);
    EXPECT_EQ(result[999], 999);
    
    // 性能应该在合理范围内
    EXPECT_LT(duration.count(), 100000); // 小于100毫秒
    
    std::cout << "大型向量解析时间: " << duration.count() << " 微秒" << std::endl;
}