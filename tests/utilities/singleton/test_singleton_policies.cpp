/**
 * @file test_singleton_policies.cpp
 * @brief Zeus Singleton Mutex策略测试
 */

#include "common/utilities/singleton.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <shared_mutex>
#include <chrono>

using namespace common::utilities;

namespace {

// 使用NullMutex的单例类
class NullMutexSingleton : public Singleton<NullMutexSingleton, NullMutex> {
    SINGLETON_FACTORY(NullMutexSingleton);
    SINGLETON_ACCESSOR(NullMutexSingleton);

public:
    void SetValue(int value) { value_ = value; }
    int GetValue() const { return value_; }
    std::string GetMutexType() const { return "NullMutex"; }

private:
    NullMutexSingleton() : value_(0) {}
    int value_;
};

// 使用ThreadSafeMutex的单例类
class ThreadSafeMutexSingleton : public Singleton<ThreadSafeMutexSingleton, ThreadSafeMutex> {
    SINGLETON_FACTORY(ThreadSafeMutexSingleton);
    SINGLETON_ACCESSOR(ThreadSafeMutexSingleton);

public:
    void SetValue(int value) { value_ = value; }
    int GetValue() const { return value_; }
    std::string GetMutexType() const { return "ThreadSafeMutex"; }

private:
    ThreadSafeMutexSingleton() : value_(0) {}
    int value_;
};

// 使用RecursiveMutex的单例类
class RecursiveMutexSingleton : public Singleton<RecursiveMutexSingleton, RecursiveMutex> {
    SINGLETON_FACTORY(RecursiveMutexSingleton);
    SINGLETON_ACCESSOR(RecursiveMutexSingleton);

public:
    void SetValue(int value) { value_ = value; }
    int GetValue() const { return value_; }
    std::string GetMutexType() const { return "RecursiveMutex"; }
    
    // 测试递归调用
    void RecursiveCall(int depth) {
        if (depth > 0) {
            value_ += depth;
            RecursiveCall(depth - 1);
        }
    }

private:
    RecursiveMutexSingleton() : value_(0) {}
    int value_;
};

// 使用SharedMutex的单例类
class SharedMutexSingleton : public Singleton<SharedMutexSingleton, SharedMutex> {
    SINGLETON_FACTORY(SharedMutexSingleton);
    SINGLETON_ACCESSOR(SharedMutexSingleton);

public:
    void SetValue(int value) { value_ = value; }
    int GetValue() const { return value_; }
    std::string GetMutexType() const { return "SharedMutex"; }

private:
    SharedMutexSingleton() : value_(0) {}
    int value_;
};

// 测试自定义Mutex策略
class TestMutex {
public:
    void lock() { 
        lock_count_.fetch_add(1, std::memory_order_relaxed);
        mutex_.lock(); 
    }
    
    void unlock() { 
        unlock_count_.fetch_add(1, std::memory_order_relaxed);
        mutex_.unlock(); 
    }
    
    bool try_lock() { 
        bool result = mutex_.try_lock();
        if (result) {
            lock_count_.fetch_add(1, std::memory_order_relaxed);
        }
        return result;
    }
    
    static std::atomic<int> lock_count_;
    static std::atomic<int> unlock_count_;

private:
    std::mutex mutex_;
};

std::atomic<int> TestMutex::lock_count_{0};
std::atomic<int> TestMutex::unlock_count_{0};

class CustomMutexSingleton : public Singleton<CustomMutexSingleton, TestMutex> {
    SINGLETON_FACTORY(CustomMutexSingleton);
    SINGLETON_ACCESSOR(CustomMutexSingleton);

public:
    void SetValue(int value) { value_ = value; }
    int GetValue() const { return value_; }
    std::string GetMutexType() const { return "TestMutex"; }

private:
    CustomMutexSingleton() : value_(0) {}
    int value_;
};

} // anonymous namespace

/**
 * @brief Mutex策略测试类
 */
class SingletonPoliciesTest : public ::testing::Test {
protected:
    void SetUp() override {
        NullMutexSingleton::Reset();
        ThreadSafeMutexSingleton::Reset();
        RecursiveMutexSingleton::Reset();
        SharedMutexSingleton::Reset();
        CustomMutexSingleton::Reset();
        TestMutex::lock_count_.store(0);
        TestMutex::unlock_count_.store(0);
    }
    
    void TearDown() override {
        NullMutexSingleton::Reset();
        ThreadSafeMutexSingleton::Reset();
        RecursiveMutexSingleton::Reset();
        SharedMutexSingleton::Reset();
        CustomMutexSingleton::Reset();
    }
};

/**
 * @brief 测试NullMutex策略
 */
TEST_F(SingletonPoliciesTest, NullMutexPolicy) {
    // 验证NullMutex是非线程安全的默认选择
    auto& instance = NullMutexSingleton::GetInstance();
    EXPECT_EQ(instance.GetMutexType(), "NullMutex");
    
    // 测试基本功能
    instance.SetValue(100);
    EXPECT_EQ(instance.GetValue(), 100);
    
    // 验证单例性
    auto& instance2 = NullMutexSingleton::Instance();
    EXPECT_EQ(&instance, &instance2);
    EXPECT_EQ(instance2.GetValue(), 100);
    
    // 验证IsInstanceCreated
    EXPECT_TRUE(NullMutexSingleton::IsInstanceCreated());
    
    NullMutexSingleton::Reset();
    EXPECT_FALSE(NullMutexSingleton::IsInstanceCreated());
}

/**
 * @brief 测试ThreadSafeMutex策略
 */
TEST_F(SingletonPoliciesTest, ThreadSafeMutexPolicy) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_accesses{0};
    
    // 多线程同时访问
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                auto& instance = ThreadSafeMutexSingleton::GetInstance();
                EXPECT_EQ(instance.GetMutexType(), "ThreadSafeMutex");
                total_accesses.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证所有访问都成功
    EXPECT_EQ(total_accesses.load(), num_threads * operations_per_thread);
    EXPECT_TRUE(ThreadSafeMutexSingleton::IsInstanceCreated());
    
    // 验证单例性
    auto& instance = ThreadSafeMutexSingleton::GetInstance();
    instance.SetValue(200);
    EXPECT_EQ(instance.GetValue(), 200);
}

/**
 * @brief 测试RecursiveMutex策略
 */
TEST_F(SingletonPoliciesTest, RecursiveMutexPolicy) {
    auto& instance = RecursiveMutexSingleton::GetInstance();
    EXPECT_EQ(instance.GetMutexType(), "RecursiveMutex");
    
    // 测试递归调用（这在标准mutex下会死锁）
    instance.RecursiveCall(5);
    // 5 + 4 + 3 + 2 + 1 = 15
    EXPECT_EQ(instance.GetValue(), 15);
    
    // 重置并再次测试
    instance.SetValue(0);
    instance.RecursiveCall(3);
    // 3 + 2 + 1 = 6
    EXPECT_EQ(instance.GetValue(), 6);
    
    // 多线程环境下测试递归调用
    const int num_threads = 5;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&instance, i]() {
            // 每个线程进行递归调用
            instance.RecursiveCall(i + 1);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证结果（初始值6 + 各线程的递归和）
    // 线程0: 1, 线程1: 1+2=3, 线程2: 1+2+3=6, 线程3: 1+2+3+4=10, 线程4: 1+2+3+4+5=15
    // 总和: 6 + 1 + 3 + 6 + 10 + 15 = 41
    EXPECT_EQ(instance.GetValue(), 41);
}

/**
 * @brief 测试SharedMutex策略
 */
TEST_F(SingletonPoliciesTest, SharedMutexPolicy) {
    auto& instance = SharedMutexSingleton::GetInstance();
    EXPECT_EQ(instance.GetMutexType(), "SharedMutex");
    
    // 基本功能测试
    instance.SetValue(300);
    EXPECT_EQ(instance.GetValue(), 300);
    
    // 多线程读写测试
    const int num_readers = 20;
    const int num_writers = 5;
    const int reads_per_thread = 50;
    const int writes_per_thread = 10;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_reads{0};
    std::atomic<int> total_writes{0};
    
    // 创建读线程
    for (int i = 0; i < num_readers; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < reads_per_thread; ++j) {
                auto& inst = SharedMutexSingleton::GetInstance();
                volatile int value = inst.GetValue(); // 防止优化
                (void)value;
                total_reads.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // 创建写线程
    for (int i = 0; i < num_writers; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < writes_per_thread; ++j) {
                auto& inst = SharedMutexSingleton::GetInstance();
                inst.SetValue(400 + i * writes_per_thread + j);
                total_writes.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证操作计数
    EXPECT_EQ(total_reads.load(), num_readers * reads_per_thread);
    EXPECT_EQ(total_writes.load(), num_writers * writes_per_thread);
    
    // 验证最终值是某个写操作的结果
    int final_value = instance.GetValue();
    EXPECT_GE(final_value, 400);
    EXPECT_LT(final_value, 500);
}

/**
 * @brief 测试自定义Mutex策略
 */
TEST_F(SingletonPoliciesTest, CustomMutexPolicy) {
    // 重置计数器
    TestMutex::lock_count_.store(0);
    TestMutex::unlock_count_.store(0);
    
    auto& instance = CustomMutexSingleton::GetInstance();
    EXPECT_EQ(instance.GetMutexType(), "TestMutex");
    
    // 验证构造时进行了锁操作
    EXPECT_GT(TestMutex::lock_count_.load(), 0);
    EXPECT_GT(TestMutex::unlock_count_.load(), 0);
    
    int initial_locks = TestMutex::lock_count_.load();
    int initial_unlocks = TestMutex::unlock_count_.load();
    
    // 进行一些操作
    instance.SetValue(500);
    EXPECT_EQ(instance.GetValue(), 500);
    
    // 多线程访问以触发更多锁操作
    const int num_threads = 10;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            auto& inst = CustomMutexSingleton::GetInstance();
            inst.SetValue(600 + i);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证锁操作增加了
    EXPECT_GT(TestMutex::lock_count_.load(), initial_locks);
    EXPECT_GT(TestMutex::unlock_count_.load(), initial_unlocks);
    
    // 验证锁和解锁操作匹配
    EXPECT_EQ(TestMutex::lock_count_.load(), TestMutex::unlock_count_.load());
}

/**
 * @brief 测试不同Mutex策略的性能对比
 */
TEST_F(SingletonPoliciesTest, MutexPolicyPerformanceComparison) {
    const int num_accesses = 10000;
    
    // 测试NullMutex性能
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_accesses; ++i) {
        auto& instance = NullMutexSingleton::GetInstance();
        (void)instance;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto null_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 测试ThreadSafeMutex性能
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_accesses; ++i) {
        auto& instance = ThreadSafeMutexSingleton::GetInstance();
        (void)instance;
    }
    end = std::chrono::high_resolution_clock::now();
    auto threadsafe_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 测试RecursiveMutex性能
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_accesses; ++i) {
        auto& instance = RecursiveMutexSingleton::GetInstance();
        (void)instance;
    }
    end = std::chrono::high_resolution_clock::now();
    auto recursive_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 测试SharedMutex性能
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_accesses; ++i) {
        auto& instance = SharedMutexSingleton::GetInstance();
        (void)instance;
    }
    end = std::chrono::high_resolution_clock::now();
    auto shared_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // 输出性能结果
    std::cout << "Mutex策略性能对比 (" << num_accesses << " 次访问):" << std::endl;
    std::cout << "NullMutex: " << null_duration.count() << " 微秒" << std::endl;
    std::cout << "ThreadSafeMutex: " << threadsafe_duration.count() << " 微秒" << std::endl;
    std::cout << "RecursiveMutex: " << recursive_duration.count() << " 微秒" << std::endl;
    std::cout << "SharedMutex: " << shared_duration.count() << " 微秒" << std::endl;
    
    // 性能断言（NullMutex应该是最快的）
    EXPECT_LE(null_duration.count(), threadsafe_duration.count());
    EXPECT_GT(threadsafe_duration.count(), 0);
    EXPECT_GT(recursive_duration.count(), 0);
    EXPECT_GT(shared_duration.count(), 0);
}

/**
 * @brief 测试Mutex策略的类型特征
 */
TEST_F(SingletonPoliciesTest, MutexPolicyTraits) {
    // 验证NullMutex被正确识别
    EXPECT_TRUE(mutex_traits<NullMutex>::is_null_mutex);
    EXPECT_FALSE(mutex_traits<ThreadSafeMutex>::is_null_mutex);
    EXPECT_FALSE(mutex_traits<RecursiveMutex>::is_null_mutex);
    EXPECT_FALSE(mutex_traits<SharedMutex>::is_null_mutex);
    
    // 验证不同策略的单例都能正常工作
    auto& null_instance = NullMutexSingleton::GetInstance();
    auto& threadsafe_instance = ThreadSafeMutexSingleton::GetInstance();
    auto& recursive_instance = RecursiveMutexSingleton::GetInstance();
    auto& shared_instance = SharedMutexSingleton::GetInstance();
    
    // 验证它们是不同的对象
    EXPECT_NE(static_cast<void*>(&null_instance), static_cast<void*>(&threadsafe_instance));
    EXPECT_NE(static_cast<void*>(&threadsafe_instance), static_cast<void*>(&recursive_instance));
    EXPECT_NE(static_cast<void*>(&recursive_instance), static_cast<void*>(&shared_instance));
    
    // 验证每个都能正确标识自己的类型
    EXPECT_EQ(null_instance.GetMutexType(), "NullMutex");
    EXPECT_EQ(threadsafe_instance.GetMutexType(), "ThreadSafeMutex");
    EXPECT_EQ(recursive_instance.GetMutexType(), "RecursiveMutex");
    EXPECT_EQ(shared_instance.GetMutexType(), "SharedMutex");
}

/**
 * @brief 测试便利别名
 */
TEST_F(SingletonPoliciesTest, ConvenienceAliases) {
    // 测试类型别名是否正确
    static_assert(std::is_same_v<NonThreadSafeSingleton<NullMutexSingleton>, 
                                Singleton<NullMutexSingleton, NullMutex>>);
    
    static_assert(std::is_same_v<ThreadSafeSingleton<ThreadSafeMutexSingleton>, 
                                Singleton<ThreadSafeMutexSingleton, ThreadSafeMutex>>);
    
    static_assert(std::is_same_v<RecursiveSingleton<RecursiveMutexSingleton>, 
                                Singleton<RecursiveMutexSingleton, RecursiveMutex>>);
    
    static_assert(std::is_same_v<SharedSingleton<SharedMutexSingleton>, 
                                Singleton<SharedMutexSingleton, SharedMutex>>);
    
    // 验证别名可以正常使用
    class AliasTestSingleton : public ThreadSafeSingleton<AliasTestSingleton> {
        THREAD_SAFE_SINGLETON_FACTORY(AliasTestSingleton);
        
    public:
        int GetId() const { return 12345; }
        
    private:
        AliasTestSingleton() = default;
    };
    
    auto& alias_instance = AliasTestSingleton::GetInstance();
    EXPECT_EQ(alias_instance.GetId(), 12345);
    EXPECT_TRUE(AliasTestSingleton::IsInstanceCreated());
    
    AliasTestSingleton::Reset();
    EXPECT_FALSE(AliasTestSingleton::IsInstanceCreated());
}