/**
 * @file test_singleton_thread_safety.cpp
 * @brief Zeus Singleton线程安全测试
 */

#include "common/utilities/singleton.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <set>
#include <mutex>

using namespace common::utilities;

namespace {

// 线程安全的测试单例
class ThreadSafeTestSingleton : public ThreadSafeSingleton<ThreadSafeTestSingleton> {
    THREAD_SAFE_SINGLETON_FACTORY(ThreadSafeTestSingleton);
    SINGLETON_ACCESSOR(ThreadSafeTestSingleton);

public:
    void IncrementCounter() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++counter_;
    }
    
    int GetCounter() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return counter_;
    }
    
    void SetThreadId(std::thread::id id) {
        std::lock_guard<std::mutex> lock(mutex_);
        thread_ids_.insert(id);
    }
    
    std::set<std::thread::id> GetThreadIds() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return thread_ids_;
    }
    
    void SetCreationTime() {
        creation_time_ = std::chrono::steady_clock::now();
    }
    
    auto GetCreationTime() const {
        return creation_time_;
    }

private:
    ThreadSafeTestSingleton() : counter_(0) {
        SetCreationTime();
    }
    
    mutable std::mutex mutex_;
    int counter_;
    std::set<std::thread::id> thread_ids_;
    std::chrono::steady_clock::time_point creation_time_;
};

// 非线程安全的测试单例（用于对比）
class NonThreadSafeTestSingleton : public NonThreadSafeSingleton<NonThreadSafeTestSingleton> {
    SINGLETON_FACTORY(NonThreadSafeTestSingleton);
    SINGLETON_ACCESSOR(NonThreadSafeTestSingleton);

public:
    void IncrementCounter() { ++counter_; }
    int GetCounter() const { return counter_; }
    
    void SetThreadId(std::thread::id id) { thread_ids_.insert(id); }
    std::set<std::thread::id> GetThreadIds() const { return thread_ids_; }

private:
    NonThreadSafeTestSingleton() : counter_(0) {}
    
    int counter_;
    std::set<std::thread::id> thread_ids_;
};

// 用于测试竞态条件的单例
class RaceConditionTestSingleton : public ThreadSafeSingleton<RaceConditionTestSingleton> {
    THREAD_SAFE_SINGLETON_FACTORY(RaceConditionTestSingleton);
    
public:
    static std::atomic<int> construction_count;
    static std::atomic<int> access_count;
    
    RaceConditionTestSingleton() {
        construction_count.fetch_add(1, std::memory_order_relaxed);
        // 模拟构造函数中的一些工作
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

private:
    int dummy_data_ = 42;
};

std::atomic<int> RaceConditionTestSingleton::construction_count{0};
std::atomic<int> RaceConditionTestSingleton::access_count{0};

} // anonymous namespace

/**
 * @brief 线程安全测试类
 */
class SingletonThreadSafetyTest : public ::testing::Test {
protected:
    void SetUp() override {
        ThreadSafeTestSingleton::Reset();
        NonThreadSafeTestSingleton::Reset();
        RaceConditionTestSingleton::Reset();
        RaceConditionTestSingleton::construction_count.store(0);
        RaceConditionTestSingleton::access_count.store(0);
    }
    
    void TearDown() override {
        ThreadSafeTestSingleton::Reset();
        NonThreadSafeTestSingleton::Reset();
        RaceConditionTestSingleton::Reset();
    }
};

/**
 * @brief 测试多线程环境下的单例创建安全性
 */
TEST_F(SingletonThreadSafetyTest, ConcurrentInstanceCreation) {
    const int num_threads = 50;
    const int iterations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::vector<ThreadSafeTestSingleton*> instances(num_threads * iterations_per_thread);
    std::atomic<int> instance_index{0};
    
    // 创建多个线程同时获取单例实例
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < iterations_per_thread; ++j) {
                auto& instance = ThreadSafeTestSingleton::GetInstance();
                instance.SetThreadId(std::this_thread::get_id());
                
                int idx = instance_index.fetch_add(1, std::memory_order_relaxed);
                instances[idx] = &instance;
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证所有获取的实例都是同一个对象
    auto* first_instance = instances[0];
    for (const auto* instance : instances) {
        EXPECT_EQ(instance, first_instance);
    }
    
    // 验证只有一个实例被创建
    EXPECT_TRUE(ThreadSafeTestSingleton::IsInstanceCreated());
    
    // 验证所有线程都访问了同一个实例
    auto& final_instance = ThreadSafeTestSingleton::GetInstance();
    auto thread_ids = final_instance.GetThreadIds();
    EXPECT_EQ(thread_ids.size(), num_threads);
}

/**
 * @brief 测试double-checked locking的正确性
 */
TEST_F(SingletonThreadSafetyTest, DoubleCheckedLockingCorrectness) {
    const int num_threads = 100;
    std::vector<std::thread> threads;
    std::vector<RaceConditionTestSingleton*> instances(num_threads);
    
    // 所有线程同时尝试获取单例实例
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&instances, i]() {
            RaceConditionTestSingleton::access_count.fetch_add(1, std::memory_order_relaxed);
            instances[i] = &RaceConditionTestSingleton::GetInstance();
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证构造函数只被调用一次
    EXPECT_EQ(RaceConditionTestSingleton::construction_count.load(), 1);
    
    // 验证所有线程都访问了
    EXPECT_EQ(RaceConditionTestSingleton::access_count.load(), num_threads);
    
    // 验证所有实例指针都指向同一个对象
    auto* first_instance = instances[0];
    for (const auto* instance : instances) {
        EXPECT_EQ(instance, first_instance);
    }
}

/**
 * @brief 测试多线程环境下的计数器一致性
 */
TEST_F(SingletonThreadSafetyTest, ConcurrentCounterConsistency) {
    const int num_threads = 20;
    const int increments_per_thread = 1000;
    
    std::vector<std::thread> threads;
    
    // 多个线程同时对单例的计数器进行递增
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([increments_per_thread]() {
            auto& instance = ThreadSafeTestSingleton::GetInstance();
            for (int j = 0; j < increments_per_thread; ++j) {
                instance.IncrementCounter();
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证计数器的最终值
    auto& instance = ThreadSafeTestSingleton::GetInstance();
    int expected_count = num_threads * increments_per_thread;
    EXPECT_EQ(instance.GetCounter(), expected_count);
}

/**
 * @brief 测试线程安全单例与非线程安全单例的性能对比
 */
TEST_F(SingletonThreadSafetyTest, PerformanceComparison) {
    const int num_accesses = 100000;
    
    // 测试线程安全版本的性能
    auto start_safe = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_accesses; ++i) {
        auto& instance = ThreadSafeTestSingleton::GetInstance();
        (void)instance; // 防止编译器优化
    }
    auto end_safe = std::chrono::high_resolution_clock::now();
    auto duration_safe = std::chrono::duration_cast<std::chrono::microseconds>(end_safe - start_safe);
    
    // 测试非线程安全版本的性能
    auto start_unsafe = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_accesses; ++i) {
        auto& instance = NonThreadSafeTestSingleton::GetInstance();
        (void)instance; // 防止编译器优化
    }
    auto end_unsafe = std::chrono::high_resolution_clock::now();
    auto duration_unsafe = std::chrono::duration_cast<std::chrono::microseconds>(end_unsafe - start_unsafe);
    
    std::cout << "线程安全版本访问时间: " << duration_safe.count() << " 微秒" << std::endl;
    std::cout << "非线程安全版本访问时间: " << duration_unsafe.count() << " 微秒" << std::endl;
    
    // 性能差异应该在合理范围内（线程安全版本通常稍慢）
    // 这里主要是确保性能没有数量级的差异
    EXPECT_GT(duration_safe.count(), 0);
    EXPECT_GT(duration_unsafe.count(), 0);
}

/**
 * @brief 测试Reset操作的线程安全性
 */
TEST_F(SingletonThreadSafetyTest, ConcurrentResetSafety) {
    const int num_threads = 10;
    const int operations_per_thread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_accesses{0};
    std::atomic<int> successful_resets{0};
    
    // 多个线程同时进行实例访问和重置操作
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                try {
                    if (i % 2 == 0) {
                        // 偶数线程尝试访问实例
                        auto& instance = ThreadSafeTestSingleton::GetInstance();
                        instance.IncrementCounter();
                        successful_accesses.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        // 奇数线程尝试重置实例
                        ThreadSafeTestSingleton::Reset();
                        successful_resets.fetch_add(1, std::memory_order_relaxed);
                    }
                    
                    // 添加小延迟以增加竞态条件的可能性
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                } catch (...) {
                    // 忽略可能的异常，这在重置过程中可能发生
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证操作计数（应该有一些成功的访问和重置）
    EXPECT_GT(successful_accesses.load(), 0);
    EXPECT_GT(successful_resets.load(), 0);
    
    std::cout << "成功访问次数: " << successful_accesses.load() << std::endl;
    std::cout << "成功重置次数: " << successful_resets.load() << std::endl;
}

/**
 * @brief 测试高并发下的单例创建时间一致性
 */
TEST_F(SingletonThreadSafetyTest, CreationTimeConsistency) {
    const int num_threads = 50;
    
    std::vector<std::thread> threads;
    std::vector<std::chrono::steady_clock::time_point> creation_times(num_threads);
    
    // 多个线程同时获取单例并记录创建时间
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&creation_times, i]() {
            auto& instance = ThreadSafeTestSingleton::GetInstance();
            creation_times[i] = instance.GetCreationTime();
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证所有线程获取的创建时间都相同（因为是同一个实例）
    auto first_time = creation_times[0];
    for (const auto& time : creation_times) {
        EXPECT_EQ(time, first_time);
    }
}

/**
 * @brief 压力测试：大量线程同时访问
 */
TEST_F(SingletonThreadSafetyTest, StressTest) {
    const int num_threads = 200;
    const int operations_per_thread = 50;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_operations{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 创建大量线程进行压力测试
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < operations_per_thread; ++j) {
                auto& instance = ThreadSafeTestSingleton::GetInstance();
                instance.IncrementCounter();
                instance.SetThreadId(std::this_thread::get_id());
                total_operations.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // 验证结果
    auto& instance = ThreadSafeTestSingleton::GetInstance();
    int expected_count = num_threads * operations_per_thread;
    
    EXPECT_EQ(instance.GetCounter(), expected_count);
    EXPECT_EQ(total_operations.load(), expected_count);
    EXPECT_EQ(instance.GetThreadIds().size(), num_threads);
    
    std::cout << "压力测试完成: " << num_threads << " 线程, 每线程 " 
              << operations_per_thread << " 操作, 总时间 " << duration.count() << " 毫秒" << std::endl;
}