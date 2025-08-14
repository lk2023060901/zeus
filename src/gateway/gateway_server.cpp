#include "gateway/gateway_server.h"
#include "common/network/zeus_network.h"
#include <iostream>
#include <random>
#include <sstream>

namespace gateway {

// GatewayServer 实现
GatewayServer::GatewayServer(boost::asio::any_io_executor executor, const GatewayConfig& config)
    : executor_(executor)
    , config_(config)
    , cleanup_timer_(std::make_unique<boost::asio::steady_timer>(executor)) {
    
#ifdef ZEUS_USE_KCP
    NETWORK_LOG_INFO("Gateway server created - Protocol: KCP, Port: {}", config_.listen_port);
#else
    NETWORK_LOG_INFO("Gateway server created - Protocol: TCP, Port: {}", config_.listen_port);
#endif
}

GatewayServer::~GatewayServer() {
    Stop();
    NETWORK_LOG_INFO("Gateway server destroyed");
}

bool GatewayServer::Start() {
    if (running_) {
        NETWORK_LOG_WARN("Gateway server already running");
        return true;
    }
    
    try {
        // 根据编译时宏选择监听协议
#ifdef ZEUS_USE_KCP
        kcp_acceptor_ = std::make_unique<common::network::KcpAcceptor>(
            executor_, config_.listen_port, config_.bind_address, config_.kcp_config);
        
        kcp_acceptor_->SetMaxConnections(config_.max_client_connections);
        
        if (!kcp_acceptor_->Start([this](std::shared_ptr<common::network::KcpConnector> conn) {
            OnClientConnection(std::static_pointer_cast<common::network::Connection>(conn));
        })) {
            NETWORK_LOG_ERROR("Failed to start KCP acceptor");
            return false;
        }
#else
        tcp_acceptor_ = std::make_unique<common::network::TcpAcceptor>(
            executor_, config_.listen_port, config_.bind_address);
        
        tcp_acceptor_->SetMaxConnections(config_.max_client_connections);
        
        if (!tcp_acceptor_->Start([this](std::shared_ptr<common::network::TcpConnector> conn) {
            OnClientConnection(std::static_pointer_cast<common::network::Connection>(conn));
        })) {
            NETWORK_LOG_ERROR("Failed to start TCP acceptor");
            return false;
        }
#endif
        
        running_ = true;
        
        // 启动清理定时器
        StartCleanupTimer();
        
#ifdef ZEUS_USE_KCP
        NETWORK_LOG_INFO("Gateway server started on {}:{} using KCP protocol", 
                        config_.bind_address, config_.listen_port);
#else
        NETWORK_LOG_INFO("Gateway server started on {}:{} using TCP protocol", 
                        config_.bind_address, config_.listen_port);
#endif
        
        return true;
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to start gateway server: {}", e.what());
        return false;
    }
}

void GatewayServer::Stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    NETWORK_LOG_INFO("Stopping gateway server...");
    
    // 停止接受新连接
    if (tcp_acceptor_) {
        tcp_acceptor_->Stop();
        tcp_acceptor_.reset();
    }
    
    if (kcp_acceptor_) {
        kcp_acceptor_->Stop();
        kcp_acceptor_.reset();
    }
    
    // 关闭所有会话
    CloseAllSessions();
    
    // 停止清理定时器
    if (cleanup_timer_) {
        cleanup_timer_->cancel();
    }
    
    NETWORK_LOG_INFO("Gateway server stopped");
}

void GatewayServer::OnClientConnection(std::shared_ptr<common::network::Connection> connection) {
    if (!running_) {
        NETWORK_LOG_WARN("Rejecting client connection - server is shutting down");
        connection->Close();
        return;
    }
    
    // 生成会话ID
    std::string session_id = GenerateSessionId();
    
    // 创建会话
    auto session = std::make_shared<GatewaySession>(session_id, connection);
    
    // 选择后端服务器
    std::string backend_endpoint = SelectBackendServer();
    if (backend_endpoint.empty()) {
        NETWORK_LOG_ERROR("No backend servers available for session {}", session_id);
        connection->Close();
        return;
    }
    
    // 连接到后端
    session->ConnectToBackend(backend_endpoint, executor_, config_);
    
    // 存储会话
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        sessions_[session_id] = session;
    }
    
    // 更新统计
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_sessions_created++;
        stats_.active_sessions = sessions_.size();
    }
    
    NETWORK_LOG_INFO("New client session created: {} -> Backend: {}", 
                     session_id, backend_endpoint);
}

std::string GatewayServer::GenerateSessionId() {
    auto session_num = next_session_id_.fetch_add(1);
    std::ostringstream oss;
    oss << "gw_session_" << session_num;
    return oss.str();
}

std::string GatewayServer::SelectBackendServer() {
    return protocol_router_.SelectBackendServer();
}

void GatewayServer::UpdateConfig(const GatewayConfig& config) {
    config_ = config;
    
    // 更新后端服务器列表到协议路由器
    protocol_router_.ClearBackendServers();
    for (const auto& server : config_.backend_servers) {
        protocol_router_.AddBackendServer(server);
    }
    
    NETWORK_LOG_INFO("Gateway configuration updated");
}

size_t GatewayServer::GetActiveSessionCount() const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    return sessions_.size();
}

std::shared_ptr<GatewaySession> GatewayServer::GetSession(const std::string& session_id) const {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    return (it != sessions_.end()) ? it->second : nullptr;
}

void GatewayServer::CloseSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        it->second->Close();
        sessions_.erase(it);
    }
}

void GatewayServer::CloseAllSessions() {
    std::lock_guard<std::mutex> lock(sessions_mutex_);
    for (auto& [session_id, session] : sessions_) {
        session->Close();
    }
    sessions_.clear();
}

// 后端服务器管理方法已移至 ProtocolRouter

void GatewayServer::StartCleanupTimer() {
    if (!cleanup_timer_) return;
    
    cleanup_timer_->expires_after(std::chrono::seconds(30));
    cleanup_timer_->async_wait([this](boost::system::error_code ec) {
        HandleCleanupTimer(ec);
    });
}

void GatewayServer::HandleCleanupTimer(boost::system::error_code ec) {
    if (ec == boost::asio::error::operation_aborted) {
        return; // 定时器被取消
    }
    
    if (ec) {
        NETWORK_LOG_WARN("Cleanup timer error: {}", ec.message());
        return;
    }
    
    if (running_) {
        CleanupInactiveSessions();
        StartCleanupTimer(); // 重启定时器
    }
}

void GatewayServer::CleanupInactiveSessions() {
    std::vector<std::string> sessions_to_remove;
    
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        auto now = std::chrono::steady_clock::now();
        
        for (const auto& [session_id, session] : sessions_) {
            if (!session->IsActive() || 
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - session->GetStats().last_activity).count() > config_.client_timeout_ms) {
                sessions_to_remove.push_back(session_id);
            }
        }
        
        // 移除非活跃会话
        for (const auto& session_id : sessions_to_remove) {
            auto it = sessions_.find(session_id);
            if (it != sessions_.end()) {
                it->second->Close();
                sessions_.erase(it);
                NETWORK_LOG_DEBUG("Cleaned up inactive session: {}", session_id);
            }
        }
    }
    
    // 更新统计
    if (!sessions_to_remove.empty()) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.active_sessions = sessions_.size();
        NETWORK_LOG_DEBUG("Cleaned up {} inactive sessions", sessions_to_remove.size());
    }
}

} // namespace gateway