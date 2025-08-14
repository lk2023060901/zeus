/**
 * @file test_field.cpp
 * @brief Zeus结构化日志Field类的单元测试
 */

#include "common/spdlog/structured/field.h"
#include <gtest/gtest.h>
#include <string>
#include <chrono>

using namespace common::spdlog::structured;

/**
 * @brief Field基本功能测试
 */
class FieldTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @brief 测试Field基本构造和访问
 */
TEST_F(FieldTest, BasicConstruction) {
    // 测试整数字段
    auto int_field = FIELD("count", 42);
    EXPECT_EQ(int_field.key(), "count");
    EXPECT_EQ(int_field.value(), 42);
    EXPECT_EQ(int_field.type(), FieldType::INT32);
    EXPECT_TRUE(int_field.is_numeric());
    EXPECT_FALSE(int_field.is_string());
    
    // 测试字符串字段
    auto string_field = FIELD("name", std::string("test"));
    EXPECT_EQ(string_field.key(), "name");
    EXPECT_EQ(string_field.value(), "test");
    EXPECT_EQ(string_field.type(), FieldType::STRING);
    EXPECT_FALSE(string_field.is_numeric());
    EXPECT_TRUE(string_field.is_string());
    
    // 测试布尔字段
    auto bool_field = FIELD("enabled", true);
    EXPECT_EQ(bool_field.key(), "enabled");
    EXPECT_EQ(bool_field.value(), true);
    EXPECT_EQ(bool_field.type(), FieldType::BOOL);
    EXPECT_TRUE(bool_field.is_bool());
    
    // 测试浮点数字段
    auto double_field = FIELD("ratio", 3.14);
    EXPECT_EQ(double_field.key(), "ratio");
    EXPECT_DOUBLE_EQ(double_field.value(), 3.14);
    EXPECT_EQ(double_field.type(), FieldType::DOUBLE);
    EXPECT_TRUE(double_field.is_numeric());
}

/**
 * @brief 测试Field类型推导
 */
TEST_F(FieldTest, TypeDeduction) {
    // 测试各种整数类型
    auto int8_field = make_field("i8", int8_t(127));
    EXPECT_EQ(int8_field.type(), FieldType::INT8);
    
    auto int16_field = make_field("i16", int16_t(32767));
    EXPECT_EQ(int16_field.type(), FieldType::INT16);
    
    auto int32_field = make_field("i32", int32_t(2147483647));
    EXPECT_EQ(int32_field.type(), FieldType::INT32);
    
    auto int64_field = make_field("i64", int64_t(9223372036854775807LL));
    EXPECT_EQ(int64_field.type(), FieldType::INT64);
    
    // 测试无符号整数类型
    auto uint8_field = make_field("u8", uint8_t(255));
    EXPECT_EQ(uint8_field.type(), FieldType::UINT8);
    
    auto uint32_field = make_field("u32", uint32_t(4294967295U));
    EXPECT_EQ(uint32_field.type(), FieldType::UINT32);
    
    // 测试浮点类型
    auto float_field = make_field("f", 3.14f);
    EXPECT_EQ(float_field.type(), FieldType::FLOAT);
    
    auto double_field = make_field("d", 3.14159265359);
    EXPECT_EQ(double_field.type(), FieldType::DOUBLE);
    
    // 测试字符串类型
    auto string_view_field = make_field("sv", std::string_view("hello"));
    EXPECT_EQ(string_view_field.type(), FieldType::STRING_VIEW);
    
    auto cstr_field = make_field("cstr", "world");
    EXPECT_EQ(cstr_field.type(), FieldType::STRING_VIEW);
}

/**
 * @brief 测试Field移动语义
 */
TEST_F(FieldTest, MoveSemantics) {
    std::string original = "test_string";
    
    // 测试移动构造
    auto field = make_field("key", std::move(original));
    EXPECT_EQ(field.key(), "key");
    EXPECT_EQ(field.value(), "test_string");
    // 原始字符串应该被移动
    EXPECT_TRUE(original.empty() || original == "test_string"); // 实现依赖
}

/**
 * @brief 测试Field容器
 */
TEST_F(FieldTest, FieldContainer) {
    auto container = make_fields(
        FIELD("id", 123),
        FIELD("name", "test"),
        FIELD("active", true)
    );
    
    EXPECT_EQ(container.size(), 3);
    
    // 测试访问各个字段
    auto& field0 = container.get<0>();
    EXPECT_EQ(field0.key(), "id");
    EXPECT_EQ(field0.value(), 123);
    
    auto& field1 = container.get<1>();
    EXPECT_EQ(field1.key(), "name");
    EXPECT_EQ(field1.value(), "test");
    
    auto& field2 = container.get<2>();
    EXPECT_EQ(field2.key(), "active");
    EXPECT_EQ(field2.value(), true);
}

/**
 * @brief 测试预定义字段
 */
TEST_F(FieldTest, PredefinedFields) {
    // 测试时间戳字段
    auto ts_field = fields::timestamp();
    EXPECT_EQ(ts_field.key(), "timestamp");
    EXPECT_EQ(ts_field.type(), FieldType::TIMESTAMP);
    
    // 测试自定义时间戳键名
    auto custom_ts = fields::timestamp("created_at");
    EXPECT_EQ(custom_ts.key(), "created_at");
    
    // 测试线程ID字段
    auto thread_field = fields::thread_id();
    EXPECT_EQ(thread_field.key(), "thread_id");
    EXPECT_TRUE(thread_field.is_numeric());
    
    // 测试消息字段
    auto msg_field = fields::message("Hello, World!");
    EXPECT_EQ(msg_field.key(), "message");
    EXPECT_EQ(msg_field.value(), "Hello, World!");
    
    // 测试级别字段
    auto level_field = fields::level("INFO");
    EXPECT_EQ(level_field.key(), "level");
    EXPECT_EQ(level_field.value(), "INFO");
}

/**
 * @brief 测试Field的字符串表示
 */
TEST_F(FieldTest, StringRepresentation) {
    // 测试数值类型
    auto int_field = FIELD("count", 42);
    EXPECT_EQ(int_field.to_string(), "42");
    
    auto double_field = FIELD("ratio", 3.14);
    EXPECT_EQ(int_field.to_string(), std::to_string(42));
    
    // 测试布尔类型
    auto bool_true = FIELD("flag", true);
    EXPECT_EQ(bool_true.to_string(), "true");
    
    auto bool_false = FIELD("flag", false);
    EXPECT_EQ(bool_false.to_string(), "false");
    
    // 测试字符串类型
    auto str_field = FIELD("text", std::string("hello"));
    EXPECT_EQ(str_field.to_string(), "hello");
    
    auto view_field = FIELD("text", std::string_view("world"));
    EXPECT_EQ(view_field.to_string(), "world");
}

/**
 * @brief 测试constexpr特性
 */
TEST_F(FieldTest, ConstexprSupport) {
    // 测试编译时字段类型推导
    constexpr auto type1 = get_field_type<int>();
    static_assert(type1 == FieldType::INT32);
    
    constexpr auto type2 = get_field_type<double>();
    static_assert(type2 == FieldType::DOUBLE);
    
    constexpr auto type3 = get_field_type<bool>();
    static_assert(type3 == FieldType::BOOL);
    
    constexpr auto type4 = get_field_type<std::string_view>();
    static_assert(type4 == FieldType::STRING_VIEW);
    
    // 测试constexpr字段构造（如果支持的话）
    constexpr auto field = make_field("test", 42);
    static_assert(field.value() == 42);
    static_assert(field.key() == "test");
}

/**
 * @brief 性能基准测试
 */
TEST_F(FieldTest, PerformanceBenchmark) {
    const int iterations = 100000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // 测试Field构造性能
    for (int i = 0; i < iterations; ++i) {
        auto field = FIELD("iteration", i);
        auto field2 = FIELD("value", 3.14 * i);
        auto field3 = FIELD("flag", i % 2 == 0);
        
        // 防止编译器优化掉
        volatile auto key = field.key();
        volatile auto val = field.value();
        (void)key; (void)val;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Field construction: " << iterations << " iterations in " 
              << duration.count() << " microseconds" << std::endl;
    std::cout << "Average per field: " << (duration.count() / iterations / 3.0) << " microseconds" << std::endl;
    
    // 期望每个Field构造时间应该很短（< 1微秒）
    EXPECT_LT(duration.count() / iterations / 3.0, 1.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}