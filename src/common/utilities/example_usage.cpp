/**
 * @file example_usage.cpp
 * @brief Zeus Common Utilities 使用示例
 * 
 * 演示如何使用字符串工具和单例功能。
 */

#include "common/utilities/string_utils.h"
#include "common/utilities/singleton.h"
#include <iostream>
#include <vector>
#include <map>
#include <chrono>

using namespace common::utilities;

// 自定义单例类示例
class ConfigManager : public Singleton<ConfigManager, NullMutex> {
    SINGLETON_FACTORY(ConfigManager);
    SINGLETON_ACCESSOR(ConfigManager);

public:
    void LoadConfig(const std::string& config_str) {
        // 解析配置字符串 "key1:value1,key2:value2"
        auto& utils = StringUtils::Instance();
        utils.ParseToMap(config_str, config_map_);
        
        std::cout << "配置已加载:" << std::endl;
        for (const auto& [key, value] : config_map_) {
            std::cout << "  " << key << " = " << value << std::endl;
        }
    }
    
    std::string GetConfig(const std::string& key) const {
        auto it = config_map_.find(key);
        return it != config_map_.end() ? it->second : "";
    }

private:
    ConfigManager() = default;
    std::map<std::string, std::string> config_map_;
};

// 线程安全单例示例
class ThreadSafeCounter : public ThreadSafeSingleton<ThreadSafeCounter> {
    THREAD_SAFE_SINGLETON_FACTORY(ThreadSafeCounter);
    SINGLETON_ACCESSOR(ThreadSafeCounter);

public:
    void Increment() { ++count_; }
    int GetCount() const { return count_; }

private:
    ThreadSafeCounter() = default;
    int count_ = 0;
};

int main() {
    std::cout << "=== Zeus Common Utilities 使用示例 ===" << std::endl;
    
    // 获取字符串工具实例
    auto& utils = StringUtils::Instance();
    
    std::cout << "\n1. 基础字符串操作:" << std::endl;
    
    // 字符串分割
    std::string text = "apple-banana-cherry-date";
    auto parts = utils.Split(text);
    std::cout << "分割 '" << text << "':" << std::endl;
    for (const auto& part : parts) {
        std::cout << "  " << part << std::endl;
    }
    
    // 字符串连接
    std::vector<std::string> words = {"hello", "world", "from", "zeus"};
    auto joined = utils.Join(words, " ");
    std::cout << "连接结果: " << joined << std::endl;
    
    std::cout << "\n2. 类型安全的容器解析:" << std::endl;
    
    // 解析到整数向量
    std::vector<int32_t> numbers;
    utils.ParseToVector("1-2-3-4-5", numbers);
    std::cout << "解析整数向量: ";
    for (int num : numbers) {
        std::cout << num << " ";
    }
    std::cout << std::endl;
    
    // 解析到浮点数向量（自动类型推导）
    auto floats = utils.ParseToVector<double>("1.5|2.7|3.9", "|");
    std::cout << "解析浮点数向量: ";
    for (double f : floats) {
        std::cout << f << " ";
    }
    std::cout << std::endl;
    
    // 解析到映射
    std::map<int, std::string> id_names;
    utils.ParseToMap("1:Alice,2:Bob,3:Charlie", id_names);
    std::cout << "解析ID->姓名映射:" << std::endl;
    for (const auto& [id, name] : id_names) {
        std::cout << "  " << id << " -> " << name << std::endl;
    }
    
    std::cout << "\n3. 智能分隔符检测:" << std::endl;
    
    std::string mixed_data = "a|b|c|d";
    auto detected = utils.DetectDelimiter(mixed_data);
    std::cout << "检测到的分隔符: '" << detected << "'" << std::endl;
    
    std::cout << "\n4. 日期时间处理:" << std::endl;
    
    // 当前时间转字符串
    auto now = std::chrono::system_clock::now();
    auto time_str = utils.TimeToString(now);
    std::cout << "当前时间: " << time_str << std::endl;
    
    // 字符串转时间（安全版本）
    std::chrono::system_clock::time_point parsed_time;
    if (utils.TryStringToTime("2025-01-01 12:00:00", parsed_time)) {
        std::cout << "解析时间成功: " << utils.TimeToString(parsed_time) << std::endl;
    }
    
    std::cout << "\n5. 输入法兼容性处理:" << std::endl;
    
    std::string chinese_text = "你好，世界！";
    if (utils.HasChinesePunctuation(chinese_text)) {
        std::cout << "检测到中文标点符号" << std::endl;
        auto normalized = utils.NormalizePunctuation(chinese_text);
        std::cout << "标准化前: " << chinese_text << std::endl;
        std::cout << "标准化后: " << normalized << std::endl;
    }
    
    std::cout << "\n6. 安全解析（不抛异常）:" << std::endl;
    
    std::vector<int> safe_result;
    if (utils.TryParseToVector("1-invalid-3", safe_result)) {
        std::cout << "解析成功" << std::endl;
    } else {
        std::cout << "解析失败，使用默认值" << std::endl;
        safe_result = utils.ParseToVectorSafe<int>("1-2-3", {0});
        std::cout << "默认值: ";
        for (int val : safe_result) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n7. 单例模式演示:" << std::endl;
    
    // 非线程安全单例
    auto& config = ConfigManager::Instance();
    config.LoadConfig("server:localhost,port:8080,timeout:30");
    std::cout << "服务器配置: " << config.GetConfig("server") << std::endl;
    
    // 线程安全单例
    auto& counter = ThreadSafeCounter::Instance();
    counter.Increment();
    counter.Increment();
    std::cout << "计数器值: " << counter.GetCount() << std::endl;
    
    std::cout << "\n8. 高级功能演示:" << std::endl;
    
    // 批量解析
    std::vector<std::string> data_list = {"1-2-3", "4-5-6", "7-8-9"};
    auto batch_result = utils.BatchParseToVector<int>(data_list);
    std::cout << "批量解析结果:" << std::endl;
    for (size_t i = 0; i < batch_result.size(); ++i) {
        std::cout << "  组" << i+1 << ": ";
        for (int val : batch_result[i]) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    }
    
    // 智能容器解析
    auto smart_vector = utils.Parse<std::vector<double>>("1.1-2.2-3.3");
    std::cout << "智能解析向量: ";
    for (double val : smart_vector) {
        std::cout << val << " ";
    }
    std::cout << std::endl;
    
    std::cout << "\n=== 示例完成 ===" << std::endl;
    
    return 0;
}