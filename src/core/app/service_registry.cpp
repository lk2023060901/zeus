#include "core/app/service_registry.h"
#include <iostream>
#include <thread>
#include <algorithm>

namespace core {
namespace app {

ServiceRegistry::~ServiceRegistry() {
    // 停止健康检查线程
    SetAutoHealthCheck(false);
    
    // 停止所有服务
    StopAllServices();
}

bool ServiceRegistry::RegisterService(std::unique_ptr<Service> service) {
    if (!service) {
        std::cerr << "Cannot register null service" << std::endl;
        return false;
    }
    
    const std::string& name = service->GetName();
    if (name.empty()) {
        std::cerr << "Service name cannot be empty" << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    if (services_.find(name) != services_.end()) {
        std::cerr << "Service '" << name << "' is already registered" << std::endl;
        return false;
    }
    
    auto entry = std::make_unique<ServiceEntry>();
    entry->service = std::move(service);
    entry->registered_at = std::chrono::steady_clock::now();
    
    services_[name] = std::move(entry);
    total_registered_services_.fetch_add(1);
    
    std::cout << "Service '" << name << "' registered successfully" << std::endl;
    return true;
}

Service* ServiceRegistry::GetService(const std::string& name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = services_.find(name);
    if (it != services_.end()) {
        return it->second->service.get();
    }
    
    return nullptr;
}

std::vector<Service*> ServiceRegistry::GetServicesByType(ServiceType type) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<Service*> result;
    
    for (const auto& pair : services_) {
        if (pair.second->service->GetType() == type) {
            result.push_back(pair.second->service.get());
        }
    }
    
    return result;
}

size_t ServiceRegistry::StartAllServices() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    size_t started_count = 0;
    
    for (const auto& pair : services_) {
        const std::string& name = pair.first;
        auto& entry = pair.second;
        
        std::lock_guard<std::mutex> service_lock(entry->service_mutex);
        
        if (!entry->service->IsRunning()) {
            total_start_attempts_.fetch_add(1);
            
            if (entry->service->Start()) {
                entry->started_at = std::chrono::steady_clock::now();
                started_count++;
                std::cout << "Service '" << name << "' started successfully" << std::endl;
            } else {
                total_start_failures_.fetch_add(1);
                std::cerr << "Failed to start service '" << name << "'" << std::endl;
            }
        }
    }
    
    std::cout << "Started " << started_count << " services" << std::endl;
    return started_count;
}

bool ServiceRegistry::StartService(const std::string& name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = services_.find(name);
    if (it == services_.end()) {
        std::cerr << "Service '" << name << "' not found" << std::endl;
        return false;
    }
    
    auto& entry = it->second;
    std::lock_guard<std::mutex> service_lock(entry->service_mutex);
    
    if (entry->service->IsRunning()) {
        std::cout << "Service '" << name << "' is already running" << std::endl;
        return true;
    }
    
    total_start_attempts_.fetch_add(1);
    
    if (entry->service->Start()) {
        entry->started_at = std::chrono::steady_clock::now();
        std::cout << "Service '" << name << "' started successfully" << std::endl;
        return true;
    } else {
        total_start_failures_.fetch_add(1);
        std::cerr << "Failed to start service '" << name << "'" << std::endl;
        return false;
    }
}

void ServiceRegistry::StopAllServices() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    for (const auto& pair : services_) {
        const std::string& name = pair.first;
        auto& entry = pair.second;
        
        std::lock_guard<std::mutex> service_lock(entry->service_mutex);
        
        if (entry->service->IsRunning()) {
            total_stop_attempts_.fetch_add(1);
            entry->service->Stop();
            std::cout << "Service '" << name << "' stopped" << std::endl;
        }
    }
}

bool ServiceRegistry::StopService(const std::string& name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = services_.find(name);
    if (it == services_.end()) {
        std::cerr << "Service '" << name << "' not found" << std::endl;
        return false;
    }
    
    auto& entry = it->second;
    std::lock_guard<std::mutex> service_lock(entry->service_mutex);
    
    if (!entry->service->IsRunning()) {
        std::cout << "Service '" << name << "' is already stopped" << std::endl;
        return true;
    }
    
    total_stop_attempts_.fetch_add(1);
    entry->service->Stop();
    std::cout << "Service '" << name << "' stopped" << std::endl;
    return true;
}

bool ServiceRegistry::RemoveService(const std::string& name) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = services_.find(name);
    if (it == services_.end()) {
        return false;
    }
    
    auto& entry = it->second;
    std::lock_guard<std::mutex> service_lock(entry->service_mutex);
    
    // 如果服务正在运行，先停止它
    if (entry->service->IsRunning()) {
        entry->service->Stop();
    }
    
    services_.erase(it);
    std::cout << "Service '" << name << "' removed" << std::endl;
    return true;
}

bool ServiceRegistry::HasService(const std::string& name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    return services_.find(name) != services_.end();
}

std::vector<std::string> ServiceRegistry::GetServiceNames() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<std::string> names;
    names.reserve(services_.size());
    
    for (const auto& pair : services_) {
        names.push_back(pair.first);
    }
    
    return names;
}

size_t ServiceRegistry::GetRunningServiceCount() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    size_t count = 0;
    
    for (const auto& pair : services_) {
        auto& entry = pair.second;
        std::lock_guard<std::mutex> service_lock(entry->service_mutex);
        
        if (entry->service->IsRunning()) {
            count++;
        }
    }
    
    return count;
}

size_t ServiceRegistry::GetTotalServiceCount() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    return services_.size();
}

std::vector<ServiceRegistry::ServiceStatus> ServiceRegistry::GetAllServiceStatus() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<ServiceStatus> status_list;
    status_list.reserve(services_.size());
    
    for (const auto& pair : services_) {
        const std::string& name = pair.first;
        auto& entry = pair.second;
        std::lock_guard<std::mutex> service_lock(entry->service_mutex);
        
        ServiceStatus status;
        status.name = name;
        status.type = entry->service->GetType();
        status.is_running = entry->service->IsRunning();
        status.start_time = entry->started_at;
        status.last_status_check = entry->last_health_check;
        
        status_list.push_back(status);
    }
    
    return status_list;
}

std::optional<ServiceRegistry::ServiceStatus> ServiceRegistry::GetServiceStatus(const std::string& name) const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    
    auto it = services_.find(name);
    if (it == services_.end()) {
        return std::nullopt;
    }
    
    auto& entry = it->second;
    std::lock_guard<std::mutex> service_lock(entry->service_mutex);
    
    ServiceStatus status;
    status.name = name;
    status.type = entry->service->GetType();
    status.is_running = entry->service->IsRunning();
    status.start_time = entry->started_at;
    status.last_status_check = entry->last_health_check;
    
    return status;
}

size_t ServiceRegistry::HealthCheck() {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    size_t healthy_count = 0;
    auto now = std::chrono::steady_clock::now();
    
    for (const auto& pair : services_) {
        const std::string& name = pair.first;
        auto& entry = pair.second;
        std::lock_guard<std::mutex> service_lock(entry->service_mutex);
        
        bool was_healthy = entry->is_healthy.load();
        bool is_currently_running = entry->service->IsRunning();
        
        entry->last_health_check = now;
        entry->is_healthy.store(is_currently_running);
        
        if (is_currently_running) {
            healthy_count++;
        }
        
        // 如果状态发生变化，记录日志
        if (was_healthy != is_currently_running) {
            if (is_currently_running) {
                std::cout << "Service '" << name << "' became healthy" << std::endl;
            } else {
                std::cerr << "Service '" << name << "' became unhealthy" << std::endl;
            }
        }
    }
    
    total_health_checks_.fetch_add(1);
    return healthy_count;
}

void ServiceRegistry::SetAutoHealthCheck(bool enable) {
    bool was_enabled = auto_health_check_enabled_.exchange(enable);
    
    if (enable && !was_enabled) {
        // 启动健康检查线程
        health_check_running_.store(true);
        health_check_thread_ = std::thread(&ServiceRegistry::HealthCheckLoop, this);
        std::cout << "Auto health check enabled" << std::endl;
    } else if (!enable && was_enabled) {
        // 停止健康检查线程
        health_check_running_.store(false);
        if (health_check_thread_.joinable()) {
            health_check_thread_.join();
        }
        std::cout << "Auto health check disabled" << std::endl;
    }
}

void ServiceRegistry::Clear() {
    // 停止健康检查
    SetAutoHealthCheck(false);
    
    // 停止所有服务
    StopAllServices();
    
    // 清空服务注册表
    std::lock_guard<std::mutex> lock(registry_mutex_);
    services_.clear();
    
    // 重置统计
    total_registered_services_.store(0);
    total_start_attempts_.store(0);
    total_start_failures_.store(0);
    total_stop_attempts_.store(0);
    total_health_checks_.store(0);
}

void ServiceRegistry::HealthCheckLoop() {
    while (health_check_running_.load()) {
        try {
            HealthCheck();
            std::this_thread::sleep_for(std::chrono::milliseconds(health_check_interval_ms_));
        } catch (const std::exception& e) {
            std::cerr << "Health check error: " << e.what() << std::endl;
        }
    }
}

bool ServiceRegistry::WaitForServiceStart(const std::string& name, uint32_t timeout_ms) {
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        auto service = GetService(name);
        if (service && service->IsRunning()) {
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return false;
}

bool ServiceRegistry::WaitForServiceStop(const std::string& name, uint32_t timeout_ms) {
    auto start_time = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(timeout_ms);
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        auto service = GetService(name);
        if (!service || !service->IsRunning()) {
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    return false;
}

} // namespace app
} // namespace core