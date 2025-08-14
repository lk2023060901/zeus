#include "gateway/protocol_router.h"
#include <algorithm>
#include <random>

namespace gateway {

ProtocolRouter::ProtocolRouter(LoadBalanceStrategy strategy) 
    : strategy_(strategy) {
}

void ProtocolRouter::AddBackendServer(const std::string& endpoint) {
    if (endpoint.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    // 避免重复添加
    auto it = std::find(backend_servers_.begin(), backend_servers_.end(), endpoint);
    if (it == backend_servers_.end()) {
        backend_servers_.push_back(endpoint);
    }
}

void ProtocolRouter::RemoveBackendServer(const std::string& endpoint) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = std::find(backend_servers_.begin(), backend_servers_.end(), endpoint);
    if (it != backend_servers_.end()) {
        backend_servers_.erase(it);
    }
}

std::vector<std::string> ProtocolRouter::GetBackendServers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backend_servers_;
}

void ProtocolRouter::ClearBackendServers() {
    std::lock_guard<std::mutex> lock(mutex_);
    backend_servers_.clear();
}

std::string ProtocolRouter::SelectBackendServer() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (backend_servers_.empty()) {
        return "";
    }

    switch (strategy_) {
        case LoadBalanceStrategy::ROUND_ROBIN:
            return SelectRoundRobin();
        
        case LoadBalanceStrategy::RANDOM:
            return SelectRandom();
        
        case LoadBalanceStrategy::LEAST_CONNECTIONS:
            return SelectLeastConnections();
        
        default:
            return SelectRoundRobin();
    }
}

size_t ProtocolRouter::GetBackendServerCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return backend_servers_.size();
}

bool ProtocolRouter::HasBackendServers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !backend_servers_.empty();
}

void ProtocolRouter::SetLoadBalanceStrategy(LoadBalanceStrategy strategy) {
    std::lock_guard<std::mutex> lock(mutex_);
    strategy_ = strategy;
}

ProtocolRouter::LoadBalanceStrategy ProtocolRouter::GetLoadBalanceStrategy() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return strategy_;
}

std::string ProtocolRouter::SelectRoundRobin() {
    // 注意：此方法应在已持有锁的情况下调用
    if (backend_servers_.empty()) {
        return "";
    }
    
    size_t index = round_robin_index_.fetch_add(1) % backend_servers_.size();
    return backend_servers_[index];
}

std::string ProtocolRouter::SelectRandom() {
    // 注意：此方法应在已持有锁的情况下调用
    if (backend_servers_.empty()) {
        return "";
    }
    
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, backend_servers_.size() - 1);
    
    return backend_servers_[dis(gen)];
}

std::string ProtocolRouter::SelectLeastConnections() {
    // 注意：此方法应在已持有锁的情况下调用
    // 简化实现 - 使用轮询算法
    return SelectRoundRobin();
}

} // namespace gateway