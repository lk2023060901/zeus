#pragma once

#include <vector>
#include <string>
#include <atomic>
#include <mutex>

namespace gateway {

/**
 * @brief 协议路由器，负责后端服务器的选择和负载均衡
 */
class ProtocolRouter {
public:
    /**
     * @brief 负载均衡策略枚举
     */
    enum class LoadBalanceStrategy {
        ROUND_ROBIN,    // 轮询
        RANDOM,         // 随机
        LEAST_CONNECTIONS // 最少连接（简化实现为轮询）
    };

    /**
     * @brief 构造函数
     * @param strategy 负载均衡策略
     */
    explicit ProtocolRouter(LoadBalanceStrategy strategy = LoadBalanceStrategy::ROUND_ROBIN);

    /**
     * @brief 析构函数
     */
    ~ProtocolRouter() = default;

    // 后端服务器管理
    void AddBackendServer(const std::string& endpoint);
    void RemoveBackendServer(const std::string& endpoint);
    std::vector<std::string> GetBackendServers() const;
    void ClearBackendServers();

    // 路由选择
    std::string SelectBackendServer();
    size_t GetBackendServerCount() const;
    bool HasBackendServers() const;

    // 负载均衡策略
    void SetLoadBalanceStrategy(LoadBalanceStrategy strategy);
    LoadBalanceStrategy GetLoadBalanceStrategy() const;

private:
    LoadBalanceStrategy strategy_;
    std::vector<std::string> backend_servers_;
    std::atomic<size_t> round_robin_index_{0};
    mutable std::mutex mutex_;

    // 内部路由算法
    std::string SelectRoundRobin();
    std::string SelectRandom();
    std::string SelectLeastConnections(); // 简化为轮询实现
};

} // namespace gateway