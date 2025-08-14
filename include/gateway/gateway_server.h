#pragma once

#include "gateway/gateway_session.h"
#include "gateway/protocol_router.h"
#include "common/network/connection.h"
#include "common/network/tcp_acceptor.h"
#include "common/network/kcp_acceptor.h"
#include "common/network/tcp_connector.h"
#include "common/network/kcp_connector.h"
#include "common/network/network_logger.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <boost/asio.hpp>

namespace gateway {

/**
 * @brief Gateway配置结构
 */
struct GatewayConfig {
    // 监听配置
    uint16_t listen_port = 8080;
    std::string bind_address = "0.0.0.0";
    
    // 协议选择已移除 - 现在完全由编译时宏 ZEUS_USE_KCP 控制
    
    // KCP特定配置
    common::network::KcpConnector::KcpConfig kcp_config = 
        common::network::KcpConnector::KcpConfig::FastMode();
    
    // 后端服务器配置
    std::vector<std::string> backend_servers;
    
    // 连接限制
    size_t max_client_connections = 10000;
    size_t max_backend_connections = 100;
    
    // 超时配置
    uint32_t client_timeout_ms = 60000;      // 1分钟客户端超时
    uint32_t backend_timeout_ms = 30000;     // 30秒后端超时
    uint32_t heartbeat_interval_ms = 30000;  // 30秒心跳间隔
    
    // 负载均衡策略已移至ProtocolRouter管理
};


/**
 * @brief Gateway服务器主类
 */
class GatewayServer {
public:
    explicit GatewayServer(boost::asio::any_io_executor executor, 
                          const GatewayConfig& config = GatewayConfig{});
    ~GatewayServer();

    // 服务器控制
    bool Start();
    void Stop();
    bool IsRunning() const { return running_; }
    
    // 配置管理
    void UpdateConfig(const GatewayConfig& config);
    const GatewayConfig& GetConfig() const { return config_; }
    
    // 会话管理
    size_t GetActiveSessionCount() const;
    std::shared_ptr<GatewaySession> GetSession(const std::string& session_id) const;
    void CloseSession(const std::string& session_id);
    void CloseAllSessions();
    
    // 后端服务器管理 - 现在由 ProtocolRouter 管理
    ProtocolRouter& GetProtocolRouter() { return protocol_router_; }
    
    // 统计信息
    struct GatewayStats {
        uint64_t total_sessions_created = 0;
        uint64_t active_sessions = 0;
        uint64_t total_messages_processed = 0;
        uint64_t total_bytes_processed = 0;
        uint64_t backend_connections_active = 0;
        uint64_t backend_connections_failed = 0;
        std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    };
    
    const GatewayStats& GetStats() const { return stats_; }

private:
    // 网络组件
    boost::asio::any_io_executor executor_;
    std::unique_ptr<common::network::TcpAcceptor> tcp_acceptor_;
    std::unique_ptr<common::network::KcpAcceptor> kcp_acceptor_;
    
    // 配置和状态
    GatewayConfig config_;
    std::atomic<bool> running_{false};
    
    // 会话管理
    std::unordered_map<std::string, std::shared_ptr<GatewaySession>> sessions_;
    mutable std::mutex sessions_mutex_;
    std::atomic<uint64_t> next_session_id_{1};
    
    // 协议路由器
    ProtocolRouter protocol_router_;
    
    // 统计信息
    GatewayStats stats_;
    mutable std::mutex stats_mutex_;
    
    // 定时器
    std::unique_ptr<boost::asio::steady_timer> cleanup_timer_;
    
    // 内部方法
    void StartAcceptor();
    void OnClientConnection(std::shared_ptr<common::network::Connection> connection);
    std::string GenerateSessionId();
    std::string SelectBackendServer();
    
    void StartCleanupTimer();
    void HandleCleanupTimer(boost::system::error_code ec);
    void CleanupInactiveSessions();
    
    void UpdateStats();
};

} // namespace gateway