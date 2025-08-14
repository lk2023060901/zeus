/**
 * @file test_singleton_basic.cpp
 * @brief Zeus Singleton基础功能测试
 */

#include "common/utilities/singleton.h"
#include <gtest/gtest.h>
#include <memory>

using namespace common::utilities;

namespace {

// 测试用的简单单例类
class TestSingleton : public Singleton<TestSingleton, NullMutex> {
    SINGLETON_FACTORY(TestSingleton);
    SINGLETON_ACCESSOR(TestSingleton);

public:
    void SetValue(int value) { value_ = value; }
    int GetValue() const { return value_; }
    
    int GetCallCount() const { return call_count_; }
    void IncrementCallCount() { ++call_count_; }

private:
    TestSingleton() : value_(0), call_count_(0) {}
    
    int value_;
    int call_count_;
};

// 使用工厂宏的单例类
class FactoryMacroSingleton : public Singleton<FactoryMacroSingleton> {
    SINGLETON_FACTORY(FactoryMacroSingleton);
    SINGLETON_ACCESSOR(FactoryMacroSingleton);
    
public:
    std::string GetName() const { return "FactoryMacroSingleton"; }
    
private:
    FactoryMacroSingleton() = default;
};

// 没有访问器宏的单例类
class NoAccessorSingleton : public Singleton<NoAccessorSingleton> {
    SINGLETON_FACTORY(NoAccessorSingleton);
    
public:
    int GetId() const { return 42; }
    
private:
    NoAccessorSingleton() = default;
};

} // anonymous namespace

/**
 * @brief Singleton基础功能测试
 */
class SingletonBasicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 重置所有单例实例以确保测试独立性
        TestSingleton::Reset();
        FactoryMacroSingleton::Reset();
        NoAccessorSingleton::Reset();
    }
    
    void TearDown() override {
        // 清理
        TestSingleton::Reset();
        FactoryMacroSingleton::Reset();
        NoAccessorSingleton::Reset();
    }
};

/**
 * @brief 测试单例实例创建和访问
 */
TEST_F(SingletonBasicTest, InstanceCreationAndAccess) {
    // 验证实例初始状态
    EXPECT_FALSE(TestSingleton::IsInstanceCreated());
    
    // 获取第一个实例
    auto& instance1 = TestSingleton::GetInstance();
    EXPECT_TRUE(TestSingleton::IsInstanceCreated());
    
    // 验证初始值
    EXPECT_EQ(instance1.GetValue(), 0);
    EXPECT_EQ(instance1.GetCallCount(), 0);
    
    // 修改实例状态
    instance1.SetValue(100);
    instance1.IncrementCallCount();
    
    // 获取第二个"实例"（应该是同一个）
    auto& instance2 = TestSingleton::GetInstance();
    
    // 验证是同一个实例
    EXPECT_EQ(&instance1, &instance2);
    EXPECT_EQ(instance2.GetValue(), 100);
    EXPECT_EQ(instance2.GetCallCount(), 1);
}

/**
 * @brief 测试单例的唯一性
 */
TEST_F(SingletonBasicTest, SingletonUniqueness) {
    auto& instance1 = TestSingleton::GetInstance();
    auto& instance2 = TestSingleton::GetInstance();
    auto& instance3 = TestSingleton::Instance(); // 使用访问器宏
    
    // 验证所有引用都指向同一个对象
    EXPECT_EQ(&instance1, &instance2);
    EXPECT_EQ(&instance2, &instance3);
    EXPECT_EQ(&instance1, &instance3);
    
    // 验证内存地址相同
    EXPECT_EQ(static_cast<void*>(&instance1), static_cast<void*>(&instance2));
    EXPECT_EQ(static_cast<void*>(&instance2), static_cast<void*>(&instance3));
}

/**
 * @brief 测试Reset功能
 */
TEST_F(SingletonBasicTest, ResetFunctionality) {
    // 创建实例并设置状态
    auto& instance1 = TestSingleton::GetInstance();
    instance1.SetValue(200);
    instance1.IncrementCallCount();
    
    EXPECT_TRUE(TestSingleton::IsInstanceCreated());
    EXPECT_EQ(instance1.GetValue(), 200);
    EXPECT_EQ(instance1.GetCallCount(), 1);
    
    // 重置单例
    TestSingleton::Reset();
    EXPECT_FALSE(TestSingleton::IsInstanceCreated());
    
    // 重新获取实例（应该是新的实例）
    auto& instance2 = TestSingleton::GetInstance();
    EXPECT_TRUE(TestSingleton::IsInstanceCreated());
    
    // 验证是新实例（状态已重置）
    EXPECT_EQ(instance2.GetValue(), 0);
    EXPECT_EQ(instance2.GetCallCount(), 0);
    
    // 验证内存地址可能不同（取决于内存分配器）
    // 注意：这不是严格要求，只是一般情况
}

/**
 * @brief 测试IsInstanceCreated状态检查
 */
TEST_F(SingletonBasicTest, IsInstanceCreatedCheck) {
    // 初始状态
    EXPECT_FALSE(TestSingleton::IsInstanceCreated());
    EXPECT_FALSE(FactoryMacroSingleton::IsInstanceCreated());
    EXPECT_FALSE(NoAccessorSingleton::IsInstanceCreated());
    
    // 创建一个实例
    TestSingleton::GetInstance();
    EXPECT_TRUE(TestSingleton::IsInstanceCreated());
    EXPECT_FALSE(FactoryMacroSingleton::IsInstanceCreated()); // 其他未创建
    EXPECT_FALSE(NoAccessorSingleton::IsInstanceCreated());
    
    // 创建另一个类型的实例
    FactoryMacroSingleton::GetInstance();
    EXPECT_TRUE(TestSingleton::IsInstanceCreated());
    EXPECT_TRUE(FactoryMacroSingleton::IsInstanceCreated());
    EXPECT_FALSE(NoAccessorSingleton::IsInstanceCreated());
    
    // 重置一个实例
    TestSingleton::Reset();
    EXPECT_FALSE(TestSingleton::IsInstanceCreated());
    EXPECT_TRUE(FactoryMacroSingleton::IsInstanceCreated());  // 其他不受影响
    EXPECT_FALSE(NoAccessorSingleton::IsInstanceCreated());
}

/**
 * @brief 测试SINGLETON_FACTORY宏
 */
TEST_F(SingletonBasicTest, SingletonFactoryMacro) {
    auto& instance = FactoryMacroSingleton::GetInstance();
    EXPECT_EQ(instance.GetName(), "FactoryMacroSingleton");
    
    // 验证友元关系正常工作（能够访问私有构造函数）
    auto& instance2 = FactoryMacroSingleton::GetInstance();
    EXPECT_EQ(&instance, &instance2);
}

/**
 * @brief 测试SINGLETON_ACCESSOR宏
 */
TEST_F(SingletonBasicTest, SingletonAccessorMacro) {
    // 测试有访问器宏的类
    auto& instance1 = TestSingleton::GetInstance();
    auto& instance2 = TestSingleton::Instance();
    
    EXPECT_EQ(&instance1, &instance2);
    
    // 测试有访问器宏的另一个类
    auto& factory_instance1 = FactoryMacroSingleton::GetInstance();
    auto& factory_instance2 = FactoryMacroSingleton::Instance();
    
    EXPECT_EQ(&factory_instance1, &factory_instance2);
}

/**
 * @brief 测试没有访问器宏的单例
 */
TEST_F(SingletonBasicTest, NoAccessorMacroSingleton) {
    auto& instance = NoAccessorSingleton::GetInstance();
    EXPECT_EQ(instance.GetId(), 42);
    
    // 确认没有Instance()方法（编译时检查）
    // NoAccessorSingleton::Instance(); // 这行如果取消注释应该编译失败
}

/**
 * @brief 测试多个不同类型的单例共存
 */
TEST_F(SingletonBasicTest, MultipleSingletonTypes) {
    // 创建不同类型的单例实例
    auto& test_instance = TestSingleton::GetInstance();
    auto& factory_instance = FactoryMacroSingleton::GetInstance();
    auto& no_accessor_instance = NoAccessorSingleton::GetInstance();
    
    // 验证它们是不同的对象
    EXPECT_NE(static_cast<void*>(&test_instance), static_cast<void*>(&factory_instance));
    EXPECT_NE(static_cast<void*>(&factory_instance), static_cast<void*>(&no_accessor_instance));
    EXPECT_NE(static_cast<void*>(&test_instance), static_cast<void*>(&no_accessor_instance));
    
    // 验证每个都正确创建
    EXPECT_TRUE(TestSingleton::IsInstanceCreated());
    EXPECT_TRUE(FactoryMacroSingleton::IsInstanceCreated());
    EXPECT_TRUE(NoAccessorSingleton::IsInstanceCreated());
    
    // 修改一个，不影响其他
    test_instance.SetValue(500);
    EXPECT_EQ(test_instance.GetValue(), 500);
    EXPECT_EQ(factory_instance.GetName(), "FactoryMacroSingleton");
    EXPECT_EQ(no_accessor_instance.GetId(), 42);
}

/**
 * @brief 测试单例的生命周期
 */
TEST_F(SingletonBasicTest, SingletonLifecycle) {
    // 1. 初始状态
    EXPECT_FALSE(TestSingleton::IsInstanceCreated());
    
    // 2. 创建实例
    {
        auto& instance = TestSingleton::GetInstance();
        instance.SetValue(999);
        EXPECT_TRUE(TestSingleton::IsInstanceCreated());
        EXPECT_EQ(instance.GetValue(), 999);
    } // instance引用超出作用域，但单例对象仍然存在
    
    // 3. 重新获取实例，应该保持状态
    {
        auto& instance = TestSingleton::GetInstance();
        EXPECT_TRUE(TestSingleton::IsInstanceCreated());
        EXPECT_EQ(instance.GetValue(), 999); // 状态保持
    }
    
    // 4. 显式重置
    TestSingleton::Reset();
    EXPECT_FALSE(TestSingleton::IsInstanceCreated());
    
    // 5. 重新创建
    auto& new_instance = TestSingleton::GetInstance();
    EXPECT_TRUE(TestSingleton::IsInstanceCreated());
    EXPECT_EQ(new_instance.GetValue(), 0); // 新实例，初始状态
}