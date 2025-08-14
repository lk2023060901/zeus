#pragma once

#include "common/network/connection.h"
#include "common/network/network_logger.h"
#include <memory>
#include <string>
#include <mutex>
#include <chrono>
#include <boost/asio.hpp>

namespace gateway {

// 前向声明
struct GatewayConfig;

/**
 * @brief Gateway会话，管理客户端到后端的连接映射
 */
class GatewaySession {
public:
    /**
     * @brief 构造函数
     * @param session_id 会话ID
     * @param client_conn 客户端连接
     */
    GatewaySession(const std::string& session_id, 
                   std::shared_ptr<common::network::Connection> client_conn);

    /**
     * @brief 析构函数
     */
    ~GatewaySession();

    // 会话管理
    const std::string& GetSessionId() const { return session_id_; }
    bool IsActive() const;
    void Close();
    
    // 客户端连接管理
    std::shared_ptr<common::network::Connection> GetClientConnection() const { return client_connection_; }
    void OnClientMessage(const std::vector<uint8_t>& data);
    void OnClientDisconnected(boost::system::error_code ec);
    
    // 后端连接管理
    void ConnectToBackend(const std::string& backend_endpoint, 
                         boost::asio::any_io_executor executor,
                         const GatewayConfig& config);
    void OnBackendMessage(const std::vector<uint8_t>& data);
    void OnBackendDisconnected(boost::system::error_code ec);
    
    // 消息转发
    void ForwardToBackend(const std::vector<uint8_t>& data);
    void ForwardToClient(const std::vector<uint8_t>& data);
    
    // 统计信息
    struct SessionStats {
        uint64_t client_messages_received = 0;
        uint64_t client_bytes_received = 0;
        uint64_t backend_messages_sent = 0;
        uint64_t backend_bytes_sent = 0;
        uint64_t backend_messages_received = 0;
        uint64_t backend_bytes_received = 0;
        uint64_t client_messages_sent = 0;
        uint64_t client_bytes_sent = 0;
        std::chrono::steady_clock::time_point created_time = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point last_activity = std::chrono::steady_clock::now();
    };
    
    const SessionStats& GetStats() const { return stats_; }

private:
    std::string session_id_;
    std::shared_ptr<common::network::Connection> client_connection_;
    std::shared_ptr<common::network::Connection> backend_connection_;
    SessionStats stats_;
    mutable std::mutex mutex_;
    bool active_ = true;
    
    void UpdateLastActivity();
};

} // namespace gateway