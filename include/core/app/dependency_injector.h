#pragma once

#include "application_types.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>
#include <typeindex>
#include <any>

namespace core {
namespace app {

/**
 * @brief 依赖注入容器类
 */
class DependencyInjector {
public:
    /**
     * @brief 构造函数
     */
    DependencyInjector() = default;
    
    /**
     * @brief 析构函数
     */
    ~DependencyInjector() = default;
    
    // 禁止拷贝和赋值
    DependencyInjector(const DependencyInjector&) = delete;
    DependencyInjector& operator=(const DependencyInjector&) = delete;
    
    /**
     * @brief 注册配置提供者
     * @tparam ConfigType 配置类型
     * @param name 提供者名称
     * @param provider 配置提供者实例
     */
    template<typename ConfigType>
    void RegisterConfigProvider(const std::string& name, std::shared_ptr<ConfigProvider<ConfigType>> provider);
    
    /**
     * @brief 获取配置提供者
     * @tparam ConfigType 配置类型
     * @param name 提供者名称
     * @return 配置提供者实例，未找到则返回nullptr
     */
    template<typename ConfigType>
    std::shared_ptr<ConfigProvider<ConfigType>> GetConfigProvider(const std::string& name) const;
    
    /**
     * @brief 注册单例服务
     * @tparam T 服务类型
     * @param instance 服务实例
     */
    template<typename T>
    void RegisterSingleton(std::shared_ptr<T> instance);
    
    /**
     * @brief 注册单例服务（带名称）
     * @tparam T 服务类型
     * @param name 服务名称
     * @param instance 服务实例
     */
    template<typename T>
    void RegisterSingleton(const std::string& name, std::shared_ptr<T> instance);
    
    /**
     * @brief 注册工厂函数
     * @tparam T 服务类型
     * @param factory 工厂函数
     */
    template<typename T>
    void RegisterFactory(std::function<std::shared_ptr<T>()> factory);
    
    /**
     * @brief 注册工厂函数（带名称）
     * @tparam T 服务类型
     * @param name 服务名称
     * @param factory 工厂函数
     */
    template<typename T>
    void RegisterFactory(const std::string& name, std::function<std::shared_ptr<T>()> factory);
    
    /**
     * @brief 解析服务实例
     * @tparam T 服务类型
     * @return 服务实例，未找到则返回nullptr
     */
    template<typename T>
    std::shared_ptr<T> Resolve() const;
    
    /**
     * @brief 解析服务实例（带名称）
     * @tparam T 服务类型
     * @param name 服务名称
     * @return 服务实例，未找到则返回nullptr
     */
    template<typename T>
    std::shared_ptr<T> Resolve(const std::string& name) const;
    
    /**
     * @brief 尝试解析配置并创建服务
     * @tparam ConfigType 配置类型
     * @param config_name 配置名称
     * @param json_config JSON配置数据
     * @return 配置实例，解析失败则返回nullopt
     */
    template<typename ConfigType>
    std::optional<ConfigType> TryResolveConfig(const std::string& config_name, const nlohmann::json& json_config) const;
    
    /**
     * @brief 检查是否已注册某个服务类型
     * @tparam T 服务类型
     * @return 是否已注册
     */
    template<typename T>
    bool IsRegistered() const;
    
    /**
     * @brief 检查是否已注册某个服务名称
     * @tparam T 服务类型
     * @param name 服务名称
     * @return 是否已注册
     */
    template<typename T>
    bool IsRegistered(const std::string& name) const;
    
    /**
     * @brief 检查配置提供者是否存在
     * @param name 提供者名称
     * @return 是否存在
     */
    bool HasConfigProvider(const std::string& name) const;
    
    /**
     * @brief 获取所有注册的配置提供者名称
     */
    std::vector<std::string> GetConfigProviderNames() const;
    
    /**
     * @brief 获取所有注册的服务类型信息
     */
    std::vector<std::string> GetRegisteredServices() const;
    
    /**
     * @brief 清空所有注册的服务和提供者
     */
    void Clear();

private:
    // 服务注册信息
    struct ServiceEntry {
        std::any instance;           // 单例实例
        std::any factory;           // 工厂函数
        bool is_singleton = false;  // 是否为单例
    };
    
    // 生成服务键
    template<typename T>
    std::string MakeServiceKey() const;
    
    template<typename T>
    std::string MakeServiceKey(const std::string& name) const;
    
    // 配置提供者存储
    std::unordered_map<std::string, std::any> config_providers_;
    
    // 服务存储
    std::unordered_map<std::string, ServiceEntry> services_;
    
    // 用于调试和诊断
    std::unordered_map<std::string, std::string> service_type_names_;
    
    mutable std::mutex mutex_;
};

// 实现模板方法

template<typename ConfigType>
void DependencyInjector::RegisterConfigProvider(const std::string& name, std::shared_ptr<ConfigProvider<ConfigType>> provider) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_providers_[name] = provider;
}

template<typename ConfigType>
std::shared_ptr<ConfigProvider<ConfigType>> DependencyInjector::GetConfigProvider(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = config_providers_.find(name);
    if (it != config_providers_.end()) {
        try {
            return std::any_cast<std::shared_ptr<ConfigProvider<ConfigType>>>(it->second);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }
    return nullptr;
}

template<typename T>
void DependencyInjector::RegisterSingleton(std::shared_ptr<T> instance) {
    RegisterSingleton<T>("", instance);
}

template<typename T>
void DependencyInjector::RegisterSingleton(const std::string& name, std::shared_ptr<T> instance) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeServiceKey<T>(name);
    
    ServiceEntry entry;
    entry.instance = instance;
    entry.is_singleton = true;
    
    services_[key] = entry;
    service_type_names_[key] = typeid(T).name();
}

template<typename T>
void DependencyInjector::RegisterFactory(std::function<std::shared_ptr<T>()> factory) {
    RegisterFactory<T>("", factory);
}

template<typename T>
void DependencyInjector::RegisterFactory(const std::string& name, std::function<std::shared_ptr<T>()> factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeServiceKey<T>(name);
    
    ServiceEntry entry;
    entry.factory = factory;
    entry.is_singleton = false;
    
    services_[key] = entry;
    service_type_names_[key] = typeid(T).name();
}

template<typename T>
std::shared_ptr<T> DependencyInjector::Resolve() const {
    return Resolve<T>("");
}

template<typename T>
std::shared_ptr<T> DependencyInjector::Resolve(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeServiceKey<T>(name);
    
    auto it = services_.find(key);
    if (it == services_.end()) {
        return nullptr;
    }
    
    const auto& entry = it->second;
    
    if (entry.is_singleton) {
        try {
            return std::any_cast<std::shared_ptr<T>>(entry.instance);
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    } else {
        try {
            auto factory = std::any_cast<std::function<std::shared_ptr<T>()>>(entry.factory);
            return factory();
        } catch (const std::bad_any_cast&) {
            return nullptr;
        }
    }
}

template<typename ConfigType>
std::optional<ConfigType> DependencyInjector::TryResolveConfig(const std::string& config_name, const nlohmann::json& json_config) const {
    auto provider = GetConfigProvider<ConfigType>(config_name);
    if (provider && provider->IsConfigPresent(json_config)) {
        return provider->LoadConfig(json_config);
    }
    return std::nullopt;
}

template<typename T>
bool DependencyInjector::IsRegistered() const {
    return IsRegistered<T>("");
}

template<typename T>
bool DependencyInjector::IsRegistered(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = MakeServiceKey<T>(name);
    return services_.find(key) != services_.end();
}

template<typename T>
std::string DependencyInjector::MakeServiceKey() const {
    return MakeServiceKey<T>("");
}

template<typename T>
std::string DependencyInjector::MakeServiceKey(const std::string& name) const {
    std::string type_name = typeid(T).name();
    if (name.empty()) {
        return type_name;
    }
    return type_name + "::" + name;
}

} // namespace app
} // namespace core