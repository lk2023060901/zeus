#pragma once

#include "application_types.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

namespace core {
namespace app {

/**
 * @brief 服务注册表，管理所有服务的生命周期
 */
class ServiceRegistry {
public:
    /**
     * @brief 构造函数
     */
    ServiceRegistry() = default;
    
    /**
     * @brief 析构函数
     */
    ~ServiceRegistry();
    
    // 禁止拷贝和赋值
    ServiceRegistry(const ServiceRegistry&) = delete;
    ServiceRegistry& operator=(const ServiceRegistry&) = delete;
    
    /**
     * @brief 注册服务
     * @param service 服务实例
     * @return 是否注册成功
     */
    bool RegisterService(std::unique_ptr<Service> service);
    
    /**
     * @brief 获取服务
     * @param name 服务名称
     * @return 服务实例指针，未找到返回nullptr
     */
    Service* GetService(const std::string& name) const;
    
    /**
     * @brief 按类型获取服务列表
     * @param type 服务类型
     * @return 服务实例指针列表
     */
    std::vector<Service*> GetServicesByType(ServiceType type) const;
    
    /**
     * @brief 启动所有服务
     * @return 启动成功的服务数量
     */
    size_t StartAllServices();
    
    /**
     * @brief 启动指定服务
     * @param name 服务名称
     * @return 是否启动成功
     */
    bool StartService(const std::string& name);
    
    /**
     * @brief 停止所有服务
     */
    void StopAllServices();
    
    /**
     * @brief 停止指定服务
     * @param name 服务名称
     * @return 是否停止成功
     */
    bool StopService(const std::string& name);
    
    /**
     * @brief 移除服务
     * @param name 服务名称
     * @return 是否移除成功
     */
    bool RemoveService(const std::string& name);
    
    /**
     * @brief 检查服务是否存在
     * @param name 服务名称
     * @return 是否存在
     */
    bool HasService(const std::string& name) const;
    
    /**
     * @brief 获取所有服务名称
     */
    std::vector<std::string> GetServiceNames() const;
    
    /**
     * @brief 获取运行中的服务数量
     */
    size_t GetRunningServiceCount() const;
    
    /**
     * @brief 获取服务总数
     */
    size_t GetTotalServiceCount() const;
    
    /**
     * @brief 服务状态信息
     */
    struct ServiceStatus {
        std::string name;
        ServiceType type;
        bool is_running;
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point last_status_check;
    };
    
    /**
     * @brief 获取所有服务状态
     */
    std::vector<ServiceStatus> GetAllServiceStatus() const;
    
    /**
     * @brief 获取指定服务状态
     * @param name 服务名称
     */
    std::optional<ServiceStatus> GetServiceStatus(const std::string& name) const;
    
    /**
     * @brief 健康检查所有服务
     * @return 健康的服务数量
     */
    size_t HealthCheck();
    
    /**
     * @brief 设置服务启动超时时间
     * @param timeout_ms 超时时间（毫秒）
     */
    void SetStartupTimeout(uint32_t timeout_ms) { startup_timeout_ms_ = timeout_ms; }
    
    /**
     * @brief 设置服务停止超时时间
     * @param timeout_ms 超时时间（毫秒）
     */
    void SetShutdownTimeout(uint32_t timeout_ms) { shutdown_timeout_ms_ = timeout_ms; }
    
    /**
     * @brief 设置健康检查间隔
     * @param interval_ms 检查间隔（毫秒）
     */
    void SetHealthCheckInterval(uint32_t interval_ms) { health_check_interval_ms_ = interval_ms; }
    
    /**
     * @brief 启用自动健康检查
     * @param enable 是否启用
     */
    void SetAutoHealthCheck(bool enable);
    
    /**
     * @brief 清空所有服务
     */
    void Clear();

private:
    // 服务条目
    struct ServiceEntry {
        std::unique_ptr<Service> service;
        std::chrono::steady_clock::time_point registered_at;
        std::chrono::steady_clock::time_point started_at;
        std::chrono::steady_clock::time_point last_health_check;
        std::atomic<bool> is_healthy{true};
        mutable std::mutex service_mutex;
    };
    
    // 健康检查线程函数
    void HealthCheckLoop();
    
    // 等待服务启动完成
    bool WaitForServiceStart(const std::string& name, uint32_t timeout_ms);
    
    // 等待服务停止完成
    bool WaitForServiceStop(const std::string& name, uint32_t timeout_ms);
    
    // 服务存储
    mutable std::mutex registry_mutex_;
    std::unordered_map<std::string, std::unique_ptr<ServiceEntry>> services_;
    
    // 配置参数
    uint32_t startup_timeout_ms_ = 10000;    // 10秒启动超时
    uint32_t shutdown_timeout_ms_ = 5000;    // 5秒停止超时
    uint32_t health_check_interval_ms_ = 30000; // 30秒健康检查间隔
    
    // 健康检查
    std::atomic<bool> auto_health_check_enabled_{false};
    std::atomic<bool> health_check_running_{false};
    std::thread health_check_thread_;
    
    // 统计信息
    std::atomic<size_t> total_registered_services_{0};
    std::atomic<size_t> total_start_attempts_{0};
    std::atomic<size_t> total_start_failures_{0};
    std::atomic<size_t> total_stop_attempts_{0};
    std::atomic<size_t> total_health_checks_{0};
};

} // namespace app
} // namespace core