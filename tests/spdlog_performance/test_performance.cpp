#include "common/spdlog/zeus_log_manager.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace common::spdlog;

class PerformanceTimer {
public:
    PerformanceTimer(const std::string& name) : name_(name) {
        start_ = std::chrono::high_resolution_clock::now();
    }
    
    ~PerformanceTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_);
        std::cout << "[" << name_ << "] 用时: " << duration.count() << "ms" << std::endl;
    }

private:
    std::string name_;
    std::chrono::high_resolution_clock::time_point start_;
};

void TestSequentialLogging() {
    std::cout << "\n=== 顺序日志记录性能测试 ===" << std::endl;
    
    const int message_count = 10000;
    
    {
        PerformanceTimer timer("顺序记录10000条INFO消息");
        auto logger = ZEUS_GET_LOGGER("performance");
        
        for (int i = 0; i < message_count; ++i) {
            ZEUS_LOG_INFO("performance", "Sequential message #{}: processing data", i);
        }
        logger->flush();
    }
}

void TestBatchLogging() {
    std::cout << "\n=== 批量日志记录性能测试 ===" << std::endl;
    
    const int message_count = 10000;
    
    {
        PerformanceTimer timer("批量记录10000条不同级别消息");
        auto logger = ZEUS_GET_LOGGER("performance");
        
        for (int i = 0; i < message_count; ++i) {
            switch (i % 5) {
                case 0:
                    ZEUS_LOG_TRACE("performance", "Trace message #{}", i);
                    break;
                case 1:
                    ZEUS_LOG_DEBUG("performance", "Debug message #{}", i);
                    break;
                case 2:
                    ZEUS_LOG_INFO("performance", "Info message #{}", i);
                    break;
                case 3:
                    ZEUS_LOG_WARN("performance", "Warning message #{}", i);
                    break;
                case 4:
                    ZEUS_LOG_ERROR("performance", "Error message #{}", i);
                    break;
            }
        }
        logger->flush();
    }
}

void TestMultiThreadLogging() {
    std::cout << "\n=== 多线程日志记录性能测试 ===" << std::endl;
    
    const int thread_count = 4;
    const int messages_per_thread = 2500;
    
    {
        PerformanceTimer timer("4线程并发记录10000条消息");
        
        std::vector<std::thread> threads;
        
        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([t, messages_per_thread]() {
                auto logger = ZEUS_GET_LOGGER("multithread");
                for (int i = 0; i < messages_per_thread; ++i) {
                    ZEUS_LOG_INFO("multithread", "Thread {} message #{}: data processing", t, i);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto logger = ZEUS_GET_LOGGER("multithread");
        logger->flush();
    }
}

void TestLargeMessageLogging() {
    std::cout << "\n=== 大消息日志记录性能测试 ===" << std::endl;
    
    const int message_count = 1000;
    std::string large_data(1024, 'X'); // 1KB数据
    
    {
        PerformanceTimer timer("记录1000条1KB大消息");
        auto logger = ZEUS_GET_LOGGER("performance");
        
        for (int i = 0; i < message_count; ++i) {
            ZEUS_LOG_INFO("performance", "Large message #{}: {}", i, large_data);
        }
        logger->flush();
    }
}

void TestDifferentLogLevels() {
    std::cout << "\n=== 不同日志级别性能测试 ===" << std::endl;
    
    const int message_count = 5000;
    auto logger = ZEUS_GET_LOGGER("level_test");
    
    // 设置为INFO级别，TRACE和DEBUG不会被记录
    logger->set_level(::spdlog::level::info);
    
    {
        PerformanceTimer timer("INFO级别下记录5000条TRACE消息（应该被过滤）");
        for (int i = 0; i < message_count; ++i) {
            ZEUS_LOG_TRACE("level_test", "Filtered trace message #{}", i);
        }
    }
    
    {
        PerformanceTimer timer("INFO级别下记录5000条INFO消息");
        for (int i = 0; i < message_count; ++i) {
            ZEUS_LOG_INFO("level_test", "Active info message #{}", i);
        }
        logger->flush();
    }
}

void TestRotationPerformance() {
    std::cout << "\n=== 文件轮换性能测试 ===" << std::endl;
    
    const int message_count = 5000;
    
    {
        PerformanceTimer timer("按小时轮换记录5000条消息");
        auto logger = ZEUS_GET_LOGGER("hourly_rotation");
        
        for (int i = 0; i < message_count; ++i) {
            ZEUS_LOG_INFO("hourly_rotation", "Hourly rotation message #{}: timestamp={}", 
                         i, std::chrono::system_clock::now().time_since_epoch().count());
            
            // 每100条消息休眠1ms模拟真实应用
            if (i % 100 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        logger->flush();
    }
}

int main() {
    std::cout << "=== Zeus Spdlog 性能测试套件 ===" << std::endl;
    
    // 初始化日志系统
    std::string config = R"({
        "global": {
            "log_level": "trace",
            "log_dir": "logs/performance"
        },
        "loggers": [
            {
                "name": "performance",
                "filename_pattern": "performance.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "multithread",
                "filename_pattern": "multithread.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "level_test",
                "filename_pattern": "level_test.log",
                "rotation_type": "daily",
                "console_output": false
            },
            {
                "name": "hourly_rotation",
                "filename_pattern": "hourly.log",
                "rotation_type": "hourly",
                "console_output": false
            }
        ]
    })";
    
    if (!ZeusLogManager::Instance().InitializeFromString(config)) {
        std::cerr << "初始化日志管理器失败" << std::endl;
        return 1;
    }
    
    std::cout << "初始化成功，开始性能测试..." << std::endl;
    
    // 运行各项性能测试
    TestSequentialLogging();
    TestBatchLogging();
    TestMultiThreadLogging();
    TestLargeMessageLogging();
    TestDifferentLogLevels();
    TestRotationPerformance();
    
    // 清理
    ZeusLogManager::Instance().Shutdown();
    
    std::cout << "\n=== 性能测试完成 ===" << std::endl;
    std::cout << "请查看 logs/performance/ 目录下的日志文件" << std::endl;
    
    return 0;
}