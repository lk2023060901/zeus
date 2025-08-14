#pragma once

#include "mutex_policy.h"
#include <memory>
#include <atomic>

namespace common {
namespace utilities {

/**
 * @brief 基于Mutex策略的单例模板基类
 * 
 * 使用策略模式支持不同的线程安全级别：
 * - NullMutex: 非线程安全，零开销（默认）
 * - ThreadSafeMutex: 线程安全
 * - RecursiveMutex: 支持递归调用的线程安全
 * - SharedMutex: 读写锁，适用于读多写少场景
 * 
 * @tparam T 继承此单例的具体类型
 * @tparam MutexPolicy Mutex策略类型，默认为NullMutex
 */
template<typename T, typename MutexPolicy = NullMutex>
class Singleton {
public:
    /**
     * @brief 获取单例实例
     * @return T类型的单例引用
     */
    static T& GetInstance() {
        // 使用double-checked locking pattern优化
        if constexpr (mutex_traits<MutexPolicy>::is_null_mutex) {
            // 非线程安全版本，直接检查和创建
            if (!instance_) {
                instance_ = std::make_unique<T>();
            }
        } else {
            // 线程安全版本，使用double-checked locking
            if (!instance_created_.load(std::memory_order_acquire)) {
                ScopedLock<MutexPolicy> lock(mutex_);
                if (!instance_created_.load(std::memory_order_relaxed)) {
                    instance_ = std::make_unique<T>();
                    instance_created_.store(true, std::memory_order_release);
                }
            }
        }
        
        return *instance_;
    }
    
    /**
     * @brief 线程安全的重置方法
     * 
     * 销毁当前实例，下次调用GetInstance()时会重新创建。
     * 注意：这个操作在多线程环境下需要谨慎使用。
     */
    static void Reset() {
        if constexpr (mutex_traits<MutexPolicy>::is_null_mutex) {
            instance_.reset();
        } else {
            ScopedLock<MutexPolicy> lock(mutex_);
            instance_.reset();
            instance_created_.store(false, std::memory_order_release);
        }
    }
    
    /**
     * @brief 检查实例是否已创建
     * @return true如果实例已创建，false否则
     */
    static bool IsInstanceCreated() {
        if constexpr (mutex_traits<MutexPolicy>::is_null_mutex) {
            return static_cast<bool>(instance_);
        } else {
            return instance_created_.load(std::memory_order_acquire);
        }
    }

protected:
    /**
     * @brief 受保护的构造函数
     * 
     * 防止外部直接实例化，只能通过GetInstance()获取实例。
     */
    Singleton() = default;
    
    /**
     * @brief 虚析构函数
     * 
     * 确保派生类的析构函数被正确调用。
     */
    virtual ~Singleton() = default;
    
    // 禁止拷贝和赋值
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    // 禁止移动构造和移动赋值
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

private:
    // 静态成员变量声明
    static std::unique_ptr<T> instance_;
    static MutexPolicy mutex_;
    
    // 原子标志，用于优化线程安全版本的性能
    static std::conditional_t<
        mutex_traits<MutexPolicy>::is_null_mutex,
        bool,  // 非线程安全版本使用普通bool
        std::atomic<bool>  // 线程安全版本使用原子bool
    > instance_created_;
};

// 静态成员定义
template<typename T, typename MutexPolicy>
std::unique_ptr<T> Singleton<T, MutexPolicy>::instance_ = nullptr;

template<typename T, typename MutexPolicy>
MutexPolicy Singleton<T, MutexPolicy>::mutex_{};

template<typename T, typename MutexPolicy>
std::conditional_t<
    mutex_traits<MutexPolicy>::is_null_mutex,
    bool,
    std::atomic<bool>
> Singleton<T, MutexPolicy>::instance_created_{false};

/**
 * @brief 便利别名定义
 */

// 线程安全单例
template<typename T>
using ThreadSafeSingleton = Singleton<T, ThreadSafeMutex>;

// 非线程安全单例（默认）
template<typename T>
using NonThreadSafeSingleton = Singleton<T, NullMutex>;

// 递归锁单例
template<typename T>
using RecursiveSingleton = Singleton<T, RecursiveMutex>;

// 读写锁单例
template<typename T>
using SharedSingleton = Singleton<T, SharedMutex>;

/**
 * @brief 单例工厂宏
 * 
 * 简化单例类的定义，自动生成必要的友元声明。
 * 
 * 使用方法：
 * class MyClass : public Singleton<MyClass> {
 *     SINGLETON_FACTORY(MyClass);
 * private:
 *     MyClass() = default;
 * };
 */
#define SINGLETON_FACTORY(ClassName) \
    friend class Singleton<ClassName>; \
    friend std::unique_ptr<ClassName> std::make_unique<ClassName>()

/**
 * @brief 线程安全单例工厂宏
 */
#define THREAD_SAFE_SINGLETON_FACTORY(ClassName) \
    friend class Singleton<ClassName, ThreadSafeMutex>; \
    friend std::unique_ptr<ClassName> std::make_unique<ClassName>()

/**
 * @brief 单例访问器宏
 * 
 * 为单例类生成静态访问方法，提供更友好的API。
 * 
 * 使用方法：
 * class MyClass : public Singleton<MyClass> {
 *     SINGLETON_ACCESSOR(MyClass);
 * };
 * 
 * // 可以使用
 * MyClass::Instance().SomeMethod();
 * // 而不是
 * MyClass::GetInstance().SomeMethod();
 */
#define SINGLETON_ACCESSOR(ClassName) \
public: \
    static ClassName& Instance() { return GetInstance(); }

} // namespace utilities
} // namespace common