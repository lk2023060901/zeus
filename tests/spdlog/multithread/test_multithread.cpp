#include "common/spdlog/zeus_log_manager.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>

using namespace common::spdlog;

std::atomic<int> message_counter{0};

void WorkerThread(int thread_id, int message_count, const std::string& logger_name) {
    auto logger = ZEUS_GET_LOGGER(logger_name);
    if (!logger) {
        std::cerr << "线程 " << thread_id << " 无法获取日志器: " << logger_name << std::endl;
        return;
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> level_dist(0, 4);
    std::uniform_int_distribution<> delay_dist(0, 10);
    
    for (int i = 0; i < message_count; ++i) {
        int current_msg = message_counter.fetch_add(1);
        
        // 随机选择日志级别
        int level = level_dist(gen);
        switch (level) {
            case 0:
                ZEUS_LOG_DEBUG(logger_name, "Thread {} Debug message #{}: processing item {}", 
                              thread_id, i, current_msg);
                break;
            case 1:
                ZEUS_LOG_INFO(logger_name, "Thread {} Info message #{}: completed task {}", 
                             thread_id, i, current_msg);
                break;
            case 2:
                ZEUS_LOG_WARN(logger_name, "Thread {} Warning message #{}: low resource {}", 
                             thread_id, i, current_msg);
                break;
            case 3:
                ZEUS_LOG_ERROR(logger_name, "Thread {} Error message #{}: operation failed {}", 
                              thread_id, i, current_msg);
                break;
            case 4:
                ZEUS_LOG_CRITICAL(logger_name, "Thread {} Critical message #{}: system failure {}", 
                                 thread_id, i, current_msg);
                break;
        }
        
        // 随机延迟模拟真实工作负载
        if (delay_dist(gen) == 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    
    std::cout << "线程 " << thread_id << " 完成，发送了 " << message_count << " 条消息" << std::endl;
}

void TestBasicMultiThreading() {
    std::cout << "\n=== 基础多线程测试 ===" << std::endl;
    
    const int thread_count = 8;
    const int messages_per_thread = 1000;
    
    std::vector<std::thread> threads;
    message_counter.store(0);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 启动多个线程同时写入日志
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back(WorkerThread, t, messages_per_thread, "multithread_basic");
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // 确保所有消息都被写入
    auto logger = ZEUS_GET_LOGGER("multithread_basic");
    logger->flush();
    
    std::cout << "✓ " << thread_count << " 个线程共发送 " << message_counter.load() 
              << " 条消息，用时 " << duration.count() << "ms" << std::endl;
}

void TestMultipleLoggers() {
    std::cout << "\n=== 多日志器多线程测试 ===" << std::endl;
    
    const int thread_count = 6;
    const int messages_per_thread = 500;
    
    std::vector<std::string> logger_names = {
        "game_engine", "network_system", "render_system", 
        "audio_system", "physics_system", "ai_system"
    };
    
    std::vector<std::thread> threads;
    message_counter.store(0);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 每个线程使用不同的日志器
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back(WorkerThread, t, messages_per_thread, logger_names[t]);
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // 刷新所有日志器
    for (const auto& logger_name : logger_names) {
        auto logger = ZEUS_GET_LOGGER(logger_name);
        if (logger) {
            logger->flush();
        }
    }
    
    std::cout << "✓ " << thread_count << " 个线程使用不同日志器共发送 " << message_counter.load() 
              << " 条消息，用时 " << duration.count() << "ms" << std::endl;
}

void StressTestThread(int thread_id, int duration_seconds) {
    auto logger = ZEUS_GET_LOGGER("stress_test");
    if (!logger) {
        std::cerr << "压力测试线程 " << thread_id << " 无法获取日志器" << std::endl;
        return;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(duration_seconds);
    
    int local_counter = 0;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> data_dist(1000, 9999);
    
    while (std::chrono::steady_clock::now() < end_time) {
        int data = data_dist(gen);
        ZEUS_LOG_INFO("stress_test", "Thread {} stress message #{}: data={}, timestamp={}", 
                     thread_id, local_counter++, data, 
                     std::chrono::system_clock::now().time_since_epoch().count());
        
        message_counter.fetch_add(1);
    }
    
    std::cout << "压力测试线程 " << thread_id << " 完成，发送了 " << local_counter << " 条消息" << std::endl;
}

void TestStressTest() {
    std::cout << "\n=== 压力测试 ===" << std::endl;
    
    const int thread_count = 10;
    const int test_duration = 5; // 5秒
    
    std::vector<std::thread> threads;
    message_counter.store(0);
    
    std::cout << "启动 " << thread_count << " 个线程进行 " << test_duration << " 秒压力测试..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 启动压力测试线程
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back(StressTestThread, t, test_duration);
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    auto logger = ZEUS_GET_LOGGER("stress_test");
    logger->flush();
    
    int total_messages = message_counter.load();
    double messages_per_second = (total_messages * 1000.0) / duration.count();
    
    std::cout << "✓ 压力测试完成：" << std::endl;
    std::cout << "  - 总消息数: " << total_messages << std::endl;
    std::cout << "  - 总耗时: " << duration.count() << "ms" << std::endl;
    std::cout << "  - 吞吐量: " << static_cast<int>(messages_per_second) << " 消息/秒" << std::endl;
}

void TestRotationConcurrency() {
    std::cout << "\n=== 文件轮换并发测试 ===" << std::endl;
    
    const int thread_count = 4;
    const int messages_per_thread = 1000;
    
    // 使用小时轮换来测试文件切换
    std::vector<std::thread> threads;
    message_counter.store(0);
    
    // 启动线程持续写入日志
    for (int t = 0; t < thread_count; ++t) {
        threads.emplace_back([t, messages_per_thread]() {
            auto logger = ZEUS_GET_LOGGER("rotation_test");
            for (int i = 0; i < messages_per_thread; ++i) {
                ZEUS_LOG_INFO("rotation_test", 
                             "Thread {} rotation message #{}: time={}", 
                             t, i, std::chrono::system_clock::now().time_since_epoch().count());
                message_counter.fetch_add(1);
                
                // 短暂延迟以模拟真实负载
                if (i % 100 == 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }
    
    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto logger = ZEUS_GET_LOGGER("rotation_test");
    logger->flush();
    
    std::cout << "✓ 文件轮换并发测试完成，共写入 " << message_counter.load() << " 条消息" << std::endl;
}

int main() {
    std::cout << "=== Zeus Spdlog 多线程测试套件 ===" << std::endl;
    
    // 初始化日志系统
    std::string config = R"({
        "global": {
            "log_level": "debug",
            "log_dir": "logs/multithread"
        },
        "loggers": [
            {
                "name": "multithread_basic",
                "filename_pattern": "basic_multithread.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "game_engine",
                "filename_pattern": "game_engine.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "network_system",
                "filename_pattern": "network_system.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "render_system",
                "filename_pattern": "render_system.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "audio_system",
                "filename_pattern": "audio_system.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "physics_system",
                "filename_pattern": "physics_system.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "ai_system",
                "filename_pattern": "ai_system.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "stress_test",
                "filename_pattern": "stress_test.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "rotation_test",
                "filename_pattern": "rotation_test.log",
                "rotation_type": "hourly",
                "console_output": false
            }
        ]
    })";
    
    if (!ZeusLogManager::Instance().InitializeFromString(config)) {
        std::cerr << "初始化日志管理器失败" << std::endl;
        return 1;
    }
    
    std::cout << "初始化成功，开始多线程测试..." << std::endl;
    
    // 运行各项多线程测试
    TestBasicMultiThreading();
    TestMultipleLoggers();
    TestStressTest();
    TestRotationConcurrency();
    
    // 清理
    ZeusLogManager::Instance().Shutdown();
    
    std::cout << "\n=== 多线程测试完成 ===" << std::endl;
    std::cout << "请查看 logs/multithread/ 目录下的日志文件" << std::endl;
    
    return 0;
}