#pragma once

#include <mutex>
#include <shared_mutex>

namespace common {
namespace utilities {

/**
 * @brief 空Mutex策略 - 非线程安全（默认）
 * 
 * 用于单线程环境或不需要线程同步的场景。
 * 提供与标准mutex相同的接口，但实际上不执行任何同步操作。
 * 这样可以在非多线程场景下获得零开销抽象。
 */
class NullMutex {
public:
    void lock() {}
    void unlock() {}
    bool try_lock() { return true; }
    
    // 支持RAII锁定
    class lock_guard {
    public:
        explicit lock_guard(NullMutex&) {}
        ~lock_guard() = default;
        lock_guard(const lock_guard&) = delete;
        lock_guard& operator=(const lock_guard&) = delete;
    };
};

/**
 * @brief 线程安全Mutex策略
 * 
 * 使用标准mutex提供线程安全保护。
 * 适用于需要线程安全的单例或其他共享资源。
 */
using ThreadSafeMutex = std::mutex;

/**
 * @brief 递归Mutex策略
 * 
 * 使用递归mutex，支持同一线程多次获取锁。
 * 适用于可能发生递归调用的场景。
 */
using RecursiveMutex = std::recursive_mutex;

/**
 * @brief 读写锁策略
 * 
 * 使用共享mutex，支持多读单写操作。
 * 适用于读多写少的场景，可以提供更好的并发性能。
 */
using SharedMutex = std::shared_mutex;

/**
 * @brief Mutex策略特征检测
 * 
 * 用于编译时检测Mutex类型，可以根据不同的Mutex策略
 * 进行编译时优化。
 */
template<typename MutexType>
struct mutex_traits {
    static constexpr bool is_null_mutex = std::is_same_v<MutexType, NullMutex>;
    static constexpr bool is_thread_safe = !is_null_mutex;
    static constexpr bool is_recursive = std::is_same_v<MutexType, RecursiveMutex>;
    static constexpr bool is_shared = std::is_same_v<MutexType, SharedMutex>;
};

/**
 * @brief 便利的锁定辅助类
 * 
 * 提供统一的锁定接口，自动适配不同的Mutex类型。
 */
template<typename MutexType>
class ScopedLock {
public:
    explicit ScopedLock(MutexType& mutex) : mutex_(mutex) {
        if constexpr (!mutex_traits<MutexType>::is_null_mutex) {
            mutex_.lock();
        }
    }
    
    ~ScopedLock() {
        if constexpr (!mutex_traits<MutexType>::is_null_mutex) {
            mutex_.unlock();
        }
    }
    
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

private:
    MutexType& mutex_;
};

/**
 * @brief 共享锁定辅助类（用于读写锁）
 * 
 * 专门用于SharedMutex的共享锁定。
 */
template<>
class ScopedLock<SharedMutex> {
public:
    explicit ScopedLock(SharedMutex& mutex, bool exclusive = true) 
        : mutex_(mutex), exclusive_(exclusive) {
        if (exclusive_) {
            mutex_.lock();
        } else {
            mutex_.lock_shared();
        }
    }
    
    ~ScopedLock() {
        if (exclusive_) {
            mutex_.unlock();
        } else {
            mutex_.unlock_shared();
        }
    }
    
    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

private:
    SharedMutex& mutex_;
    bool exclusive_;
};

} // namespace utilities
} // namespace common