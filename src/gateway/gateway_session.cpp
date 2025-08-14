#include "gateway/gateway_session.h"
#include "gateway/gateway_server.h"
#include "common/network/tcp_connector.h"
#include "common/network/kcp_connector.h"
#include <iostream>

namespace gateway {

// GatewaySession 实现
GatewaySession::GatewaySession(const std::string& session_id, 
                               std::shared_ptr<common::network::Connection> client_conn)
    : session_id_(session_id)
    , client_connection_(client_conn)
    , active_(true) {
    
    // 设置客户端连接的事件处理器
    if (client_connection_) {
        client_connection_->SetDataHandler([this](const std::vector<uint8_t>& data) {
            OnClientMessage(data);
        });
        
        client_connection_->SetErrorHandler([this](boost::system::error_code ec) {
            OnClientDisconnected(ec);
        });
    }
    
    NETWORK_LOG_INFO("Gateway session created: {}", session_id_);
}

GatewaySession::~GatewaySession() {
    Close();
    NETWORK_LOG_INFO("Gateway session destroyed: {}", session_id_);
}

bool GatewaySession::IsActive() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_ && client_connection_ && client_connection_->IsConnected();
}

void GatewaySession::Close() {
    std::lock_guard<std::mutex> lock(mutex_);
    active_ = false;
    
    if (client_connection_) {
        client_connection_->Close();
    }
    
    if (backend_connection_) {
        backend_connection_->Close();
    }
    
    NETWORK_LOG_INFO("Gateway session closed: {}", session_id_);
}

void GatewaySession::OnClientMessage(const std::vector<uint8_t>& data) {
    UpdateLastActivity();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.client_messages_received++;
        stats_.client_bytes_received += data.size();
    }
    
    NETWORK_LOG_TRACE("Client message received in session {}: {} bytes", session_id_, data.size());
    
    // 转发到后端
    ForwardToBackend(data);
}

void GatewaySession::OnClientDisconnected(boost::system::error_code ec) {
    NETWORK_LOG_INFO("Client disconnected from session {}: {}", session_id_, ec.message());
    Close();
}

void GatewaySession::ConnectToBackend(const std::string& backend_endpoint, 
                                     boost::asio::any_io_executor executor,
                                     const GatewayConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!active_) {
        NETWORK_LOG_WARN("Cannot connect to backend - session {} is inactive", session_id_);
        return;
    }
    
    try {
        // 根据编译时宏选择协议
#ifdef ZEUS_USE_KCP
        auto kcp_conn = std::make_shared<common::network::KcpConnector>(
            executor, session_id_ + "_backend", config.kcp_config);
        backend_connection_ = kcp_conn;
#else
        auto tcp_conn = std::make_shared<common::network::TcpConnector>(
            executor, session_id_ + "_backend");
        backend_connection_ = tcp_conn;
#endif
        
        // 设置后端连接的事件处理器
        backend_connection_->SetDataHandler([this](const std::vector<uint8_t>& data) {
            OnBackendMessage(data);
        });
        
        backend_connection_->SetErrorHandler([this](boost::system::error_code ec) {
            OnBackendDisconnected(ec);
        });
        
        // 连接到后端
        backend_connection_->AsyncConnect(backend_endpoint, 
            [this, backend_endpoint](boost::system::error_code ec) {
                if (ec) {
                    NETWORK_LOG_ERROR("Failed to connect to backend {} for session {}: {}", 
                                    backend_endpoint, session_id_, ec.message());
                } else {
                    NETWORK_LOG_INFO("Connected to backend {} for session {}", 
                                    backend_endpoint, session_id_);
                }
            });
            
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Exception connecting to backend for session {}: {}", session_id_, e.what());
    }
}

void GatewaySession::OnBackendMessage(const std::vector<uint8_t>& data) {
    UpdateLastActivity();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.backend_messages_received++;
        stats_.backend_bytes_received += data.size();
    }
    
    NETWORK_LOG_TRACE("Backend message received in session {}: {} bytes", session_id_, data.size());
    
    // 转发到客户端
    ForwardToClient(data);
}

void GatewaySession::OnBackendDisconnected(boost::system::error_code ec) {
    NETWORK_LOG_WARN("Backend disconnected from session {}: {}", session_id_, ec.message());
    // 可以实现重连逻辑，这里简单关闭会话
    Close();
}

void GatewaySession::ForwardToBackend(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!backend_connection_ || !backend_connection_->IsConnected()) {
        NETWORK_LOG_WARN("Cannot forward to backend - no active backend connection in session {}", session_id_);
        return;
    }
    
    backend_connection_->AsyncSend(data, [this](boost::system::error_code ec, size_t bytes_sent) {
        if (ec) {
            NETWORK_LOG_ERROR("Failed to forward message to backend in session {}: {}", session_id_, ec.message());
        } else {
            stats_.backend_messages_sent++;
            stats_.backend_bytes_sent += bytes_sent;
            NETWORK_LOG_TRACE("Forwarded {} bytes to backend in session {}", bytes_sent, session_id_);
        }
    });
}

void GatewaySession::ForwardToClient(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!client_connection_ || !client_connection_->IsConnected()) {
        NETWORK_LOG_WARN("Cannot forward to client - no active client connection in session {}", session_id_);
        return;
    }
    
    client_connection_->AsyncSend(data, [this](boost::system::error_code ec, size_t bytes_sent) {
        if (ec) {
            NETWORK_LOG_ERROR("Failed to forward message to client in session {}: {}", session_id_, ec.message());
        } else {
            stats_.client_messages_sent++;
            stats_.client_bytes_sent += bytes_sent;
            NETWORK_LOG_TRACE("Forwarded {} bytes to client in session {}", bytes_sent, session_id_);
        }
    });
}

void GatewaySession::UpdateLastActivity() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.last_activity = std::chrono::steady_clock::now();
}

} // namespace gateway