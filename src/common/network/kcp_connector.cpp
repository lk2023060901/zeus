#include "common/network/kcp_connector.h"
#include "common/network/network_logger.h"
#include <boost/asio.hpp>
#include <sstream>
#include <cstring>

namespace common {
namespace network {

namespace {
    // Get current timestamp in milliseconds
    uint32_t GetCurrentTimeMs() {
        auto now = std::chrono::steady_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
}

KcpConnector::KcpConnector(boost::asio::any_io_executor executor,
                          const std::string& connection_id,
                          const KcpConfig& config)
    : Connection(executor, connection_id)
    , config_(config)
    , socket_(std::make_shared<socket_type>(executor))
    , resolver_(std::make_unique<resolver_type>(executor))
    , socket_owned_(true)
    , connected_(false)
    , connecting_(false)
    , heartbeat_timer_(std::make_unique<boost::asio::steady_timer>(executor))
    , last_update_time_(std::chrono::steady_clock::now())
    , connection_start_time_(std::chrono::steady_clock::now())
    , handshake_sent_(false)
    , handshake_completed_(false)
    , bytes_sent_(0)
    , bytes_received_(0)
    , packets_sent_(0)
    , packets_received_(0) {
    
    // Initialize KCP with conversation ID from config
    InitializeKcp();
    
    // Reserve space for receive buffer
    kcp_receive_buffer_.reserve(4096);
    
    NETWORK_LOG_DEBUG("KcpConnector created for client connection: {}", connection_id);
}

KcpConnector::KcpConnector(std::shared_ptr<socket_type> socket,
                          const endpoint_type& endpoint,
                          const std::string& connection_id,
                          const KcpConfig& config)
    : Connection(socket->get_executor(), connection_id)
    , config_(config)
    , socket_(socket)
    , remote_endpoint_(endpoint)
    , socket_owned_(false)
    , connected_(true)  // Server-side connections start as connected
    , connecting_(false)
    , heartbeat_timer_(std::make_unique<boost::asio::steady_timer>(socket->get_executor()))
    , last_update_time_(std::chrono::steady_clock::now())
    , connection_start_time_(std::chrono::steady_clock::now())
    , handshake_sent_(false)
    , handshake_completed_(true)  // Server-side connections complete handshake during creation
    , bytes_sent_(0)
    , bytes_received_(0)
    , packets_sent_(0)
    , packets_received_(0) {
    
    // Get local endpoint from socket
    boost::system::error_code ec;
    local_endpoint_ = socket_->local_endpoint(ec);
    if (ec) {
        NETWORK_LOG_WARN("Failed to get local endpoint: {}", ec.message());
    }
    
    // Initialize KCP with conversation ID from config
    InitializeKcp();
    
    // Reserve space for receive buffer
    kcp_receive_buffer_.reserve(4096);
    
    // Start heartbeat timer
    StartHeartbeatTimer();
    
    NETWORK_LOG_DEBUG("KcpConnector created for server-side connection: {} -> {}", 
                     GetLocalEndpoint(), GetRemoteEndpoint());
}

KcpConnector::~KcpConnector() {
    // Close connection and cleanup
    ForceClose();
    
    // Destroy KCP instance
    DestroyKcp();
    
    NETWORK_LOG_DEBUG("KcpConnector destroyed: {}", GetConnectionId());
}

void KcpConnector::InitializeKcp() {
    // Create KCP instance with conversation ID
    kcp_ = ikcp_create(config_.conv_id, this);
    if (!kcp_) {
        NETWORK_LOG_ERROR("Failed to create KCP instance for connection: {}", GetConnectionId());
        return;
    }
    
    // Set KCP output callback
    ikcp_setoutput(kcp_, &KcpConnector::KcpOutputCallback);
    
    // Configure KCP parameters
    ConfigureKcp();
    
    NETWORK_LOG_DEBUG("KCP initialized for connection: {}, conv_id: {}", GetConnectionId(), config_.conv_id);
}

void KcpConnector::DestroyKcp() {
    if (kcp_) {
        ikcp_release(kcp_);
        kcp_ = nullptr;
    }
}

void KcpConnector::ConfigureKcp() {
    if (!kcp_) return;
    
    // Configure KCP parameters
    ikcp_nodelay(kcp_, config_.nodelay, config_.interval, config_.resend, config_.nc);
    ikcp_wndsize(kcp_, config_.sndwnd, config_.rcvwnd);
    ikcp_setmtu(kcp_, config_.mtu);
    
    NETWORK_LOG_DEBUG("KCP configured - nodelay:{}, interval:{}, resend:{}, nc:{}, sndwnd:{}, rcvwnd:{}, mtu:{}", 
                     config_.nodelay, config_.interval, config_.resend, config_.nc, 
                     config_.sndwnd, config_.rcvwnd, config_.mtu);
}

int KcpConnector::KcpOutputCallback(const char* buf, int len, ikcpcb* kcp, void* user) {
    auto* connector = static_cast<KcpConnector*>(user);
    if (!connector || !connector->socket_) {
        return -1;
    }
    
    // Copy data to vector for handling
    std::vector<uint8_t> data(buf, buf + len);
    connector->HandleKcpOutput(data);
    
    return 0;
}

void KcpConnector::HandleKcpOutput(const std::vector<uint8_t>& data) {
    if (!connected_ && !connecting_) {
        NETWORK_LOG_WARN("Dropping KCP output - not connected: {}", GetConnectionId());
        return;
    }
    
    // Send UDP packet to remote endpoint
    socket_->async_send_to(boost::asio::buffer(data), remote_endpoint_,
        [this, data](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                NETWORK_LOG_ERROR("Failed to send KCP output: {} - {}", ec.message(), GetConnectionId());
                HandleError(ec);
                return;
            }
            
            // Update statistics
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                bytes_sent_ += bytes_transferred;
                packets_sent_++;
            }
            
            NETWORK_LOG_TRACE("KCP output sent: {} bytes to {}", bytes_transferred, GetRemoteEndpoint());
        });
}

std::string KcpConnector::GetRemoteEndpoint() const {
    return EndpointToString(remote_endpoint_);
}

std::string KcpConnector::GetLocalEndpoint() const {
    return EndpointToString(local_endpoint_);
}

void KcpConnector::AsyncConnect(const std::string& endpoint, 
                               std::function<void(boost::system::error_code)> callback) {
    if (connected_ || connecting_) {
        NETWORK_LOG_WARN("Already connected or connecting: {}", GetConnectionId());
        if (callback) {
            boost::asio::post(socket_->get_executor(), 
                [callback]() { callback(boost::asio::error::already_connected); });
        }
        return;
    }
    
    // Parse endpoint (host:port format)
    auto colon_pos = endpoint.find_last_of(':');
    if (colon_pos == std::string::npos) {
        NETWORK_LOG_ERROR("Invalid endpoint format: {}", endpoint);
        if (callback) {
            boost::asio::post(socket_->get_executor(), 
                [callback]() { callback(boost::asio::error::invalid_argument); });
        }
        return;
    }
    
    std::string host = endpoint.substr(0, colon_pos);
    std::string port = endpoint.substr(colon_pos + 1);
    
    connecting_ = true;
    connection_start_time_ = std::chrono::steady_clock::now();
    UpdateState(ConnectionState::CONNECTING);
    
    NETWORK_LOG_INFO("Connecting to {}:{} - {}", host, port, GetConnectionId());
    
    DoConnect(host, port, callback);
}

void KcpConnector::DoConnect(const std::string& host, const std::string& port,
                            std::function<void(boost::system::error_code)> callback) {
    
    resolver_->async_resolve(host, port,
        [this, callback](boost::system::error_code ec, resolver_type::results_type endpoints) {
            HandleResolve(ec, endpoints, callback);
        });
}

void KcpConnector::HandleResolve(boost::system::error_code ec, resolver_type::results_type endpoints,
                                std::function<void(boost::system::error_code)> callback) {
    if (ec) {
        NETWORK_LOG_ERROR("Failed to resolve {}:{} - {}", ec.message(), GetConnectionId());
        connecting_ = false;
        if (callback) callback(ec);
        return;
    }
    
    // Use the first resolved endpoint
    remote_endpoint_ = *endpoints.begin();
    
    // Open UDP socket
    socket_->open(remote_endpoint_.protocol(), ec);
    if (ec) {
        NETWORK_LOG_ERROR("Failed to open UDP socket: {} - {}", ec.message(), GetConnectionId());
        connecting_ = false;
        if (callback) callback(ec);
        return;
    }
    
    // Get local endpoint
    local_endpoint_ = socket_->local_endpoint(ec);
    if (ec) {
        NETWORK_LOG_WARN("Failed to get local endpoint: {} - {}", ec.message(), GetConnectionId());
    }
    
    // Start receive loop
    StartReceiveLoop();
    
    // Send handshake
    SendHandshake();
    
    // Set connected state
    connected_ = true;
    connecting_ = false;
    UpdateState(ConnectionState::CONNECTED);
    
    // Start heartbeat timer
    StartHeartbeatTimer();
    
    NETWORK_LOG_INFO("Connected: {} -> {} - {}", GetLocalEndpoint(), GetRemoteEndpoint(), GetConnectionId());
    
    if (callback) {
        callback(boost::system::error_code{});
    }
}

void KcpConnector::StartReceiveLoop() {
    if (!socket_) return;
    
    socket_->async_receive_from(
        boost::asio::buffer(udp_receive_buffer_), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_transferred) {
            HandleUdpReceive(ec, bytes_transferred);
        });
}

void KcpConnector::HandleUdpReceive(boost::system::error_code ec, size_t bytes_transferred) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted) {
            NETWORK_LOG_ERROR("UDP receive error: {} - {}", ec.message(), GetConnectionId());
            HandleError(ec);
        }
        return;
    }
    
    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        bytes_received_ += bytes_transferred;
        packets_received_++;
    }
    
    NETWORK_LOG_TRACE("UDP packet received: {} bytes from {}", bytes_transferred, GetRemoteEndpoint());
    
    // Process received data through KCP
    std::vector<uint8_t> data(udp_receive_buffer_.begin(), udp_receive_buffer_.begin() + bytes_transferred);
    Input(data);
    
    // Continue receiving
    if (connected_) {
        StartReceiveLoop();
    }
}

void KcpConnector::Input(const std::vector<uint8_t>& data) {
    if (!kcp_ || data.empty()) return;
    
    // Input data to KCP
    int ret = ikcp_input(kcp_, reinterpret_cast<const char*>(data.data()), data.size());
    if (ret < 0) {
        NETWORK_LOG_WARN("KCP input error: {} - {}", ret, GetConnectionId());
        return;
    }
    
    // Process received KCP data
    ProcessKcpData();
}

void KcpConnector::ProcessKcpData() {
    if (!kcp_) return;
    
    char buffer[4096];
    int recv_len;
    
    while ((recv_len = ikcp_recv(kcp_, buffer, sizeof(buffer))) > 0) {
        // Copy data to receive buffer
        kcp_receive_buffer_.assign(buffer, buffer + recv_len);
        
        NETWORK_LOG_TRACE("KCP data received: {} bytes - {}", recv_len, GetConnectionId());
        
        // Notify data received
        HandleDataReceived(kcp_receive_buffer_);
    }
}

void KcpConnector::AsyncSend(const std::vector<uint8_t>& data, SendCallback callback) {
    if (!connected_ || !kcp_) {
        NETWORK_LOG_WARN("Cannot send - not connected: {}", GetConnectionId());
        if (callback) {
            boost::asio::post(socket_->get_executor(), 
                [callback]() { callback(boost::asio::error::not_connected, 0); });
        }
        return;
    }
    
    // Send through KCP
    int ret = ikcp_send(kcp_, reinterpret_cast<const char*>(data.data()), data.size());
    if (ret < 0) {
        NETWORK_LOG_ERROR("KCP send failed: {} - {}", ret, GetConnectionId());
        if (callback) {
            boost::asio::post(socket_->get_executor(), 
                [callback]() { callback(boost::asio::error::no_buffer_space, 0); });
        }
        return;
    }
    
    // Update KCP to trigger output
    Update(GetCurrentTimeMs());
    
    NETWORK_LOG_TRACE("Data sent through KCP: {} bytes - {}", data.size(), GetConnectionId());
    
    if (callback) {
        boost::asio::post(socket_->get_executor(), 
            [callback, data]() { callback(boost::system::error_code{}, data.size()); });
    }
}

void KcpConnector::Update(uint32_t current_time_ms) {
    if (!kcp_) return;
    
    ikcp_update(kcp_, current_time_ms);
    last_update_time_ = std::chrono::steady_clock::now();
}

uint32_t KcpConnector::Check(uint32_t current_time_ms) const {
    if (!kcp_) return current_time_ms + 100; // Default check in 100ms
    
    return ikcp_check(kcp_, current_time_ms);
}

void KcpConnector::SendHandshake() {
    // Simple handshake - just send a magic packet
    std::vector<uint8_t> handshake_data = {0x12, 0x34, 0x56, 0x78}; // Magic bytes
    
    socket_->async_send_to(boost::asio::buffer(handshake_data), remote_endpoint_,
        [this](boost::system::error_code ec, std::size_t) {
            if (ec) {
                NETWORK_LOG_ERROR("Failed to send handshake: {} - {}", ec.message(), GetConnectionId());
                return;
            }
            
            handshake_sent_ = true;
            NETWORK_LOG_DEBUG("Handshake sent - {}", GetConnectionId());
        });
}

void KcpConnector::HandleHandshakeResponse(const std::vector<uint8_t>& data) {
    // Simple handshake validation
    if (data.size() >= 4 && data[0] == 0x87 && data[1] == 0x65 && data[2] == 0x43 && data[3] == 0x21) {
        handshake_completed_ = true;
        NETWORK_LOG_DEBUG("Handshake completed - {}", GetConnectionId());
    }
}

void KcpConnector::SendHeartbeat() {
    if (!connected_) return;
    
    // Send a simple heartbeat packet
    std::vector<uint8_t> heartbeat_data = {0xFF, 0xFE, 0xFD, 0xFC}; // Heartbeat magic
    AsyncSend(heartbeat_data);
    
    NETWORK_LOG_TRACE("Heartbeat sent - {}", GetConnectionId());
}

void KcpConnector::StartHeartbeatTimer() {
    if (!heartbeat_timer_) return;
    
    heartbeat_timer_->expires_after(std::chrono::milliseconds(config_.heartbeat_interval));
    heartbeat_timer_->async_wait([this](boost::system::error_code ec) {
        HandleHeartbeatTimer(ec);
    });
}

void KcpConnector::HandleHeartbeatTimer(boost::system::error_code ec) {
    if (ec == boost::asio::error::operation_aborted) {
        return; // Timer was cancelled
    }
    
    if (ec) {
        NETWORK_LOG_WARN("Heartbeat timer error: {} - {}", ec.message(), GetConnectionId());
        return;
    }
    
    if (connected_) {
        SendHeartbeat();
        
        // Update KCP
        Update(GetCurrentTimeMs());
        
        // Restart timer
        StartHeartbeatTimer();
    }
}

void KcpConnector::Close() {
    if (!connected_) return;
    
    connected_ = false;
    
    NETWORK_LOG_INFO("Closing KCP connection: {}", GetConnectionId());
    
    // Cancel timers
    if (heartbeat_timer_) {
        heartbeat_timer_->cancel();
    }
    
    // Close socket if we own it
    CloseSocket();
    
    // Update state to disconnected
    UpdateState(ConnectionState::DISCONNECTED);
}

void KcpConnector::ForceClose() {
    connected_ = false;
    connecting_ = false;
    
    NETWORK_LOG_INFO("Force closing KCP connection: {}", GetConnectionId());
    
    // Cancel all timers
    if (heartbeat_timer_) {
        heartbeat_timer_->cancel();
    }
    
    // Close socket
    CloseSocket();
}

void KcpConnector::CloseSocket() {
    if (socket_ && socket_owned_) {
        boost::system::error_code ec;
        socket_->close(ec);
        if (ec) {
            NETWORK_LOG_WARN("Error closing socket: {} - {}", ec.message(), GetConnectionId());
        }
    }
}

std::string KcpConnector::EndpointToString(const endpoint_type& ep) const {
    std::ostringstream oss;
    oss << ep.address().to_string() << ":" << ep.port();
    return oss.str();
}

KcpConnector::KcpStats KcpConnector::GetKcpStats() const {
    KcpStats stats;
    
    if (!kcp_) return stats;
    
    // Extract KCP internal statistics
    stats.snd_una = kcp_->snd_una;
    stats.snd_nxt = kcp_->snd_nxt;
    stats.rcv_nxt = kcp_->rcv_nxt;
    stats.ssthresh = kcp_->ssthresh;
    stats.rto = kcp_->rx_rto;
    stats.cwnd = kcp_->cwnd;
    stats.probe = kcp_->probe;
    stats.current = kcp_->current;
    stats.interval = kcp_->interval;
    stats.ts_flush = kcp_->ts_flush;
    stats.nsnd_buf = kcp_->nsnd_buf;
    stats.nrcv_buf = kcp_->nrcv_buf;
    stats.nrcv_que = kcp_->nrcv_que;
    stats.nsnd_que = kcp_->nsnd_que;
    
    // Fill connection-level statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats.packets_sent = packets_sent_;
        stats.packets_received = packets_received_;
        stats.bytes_sent = bytes_sent_;
        stats.bytes_received = bytes_received_;
        // Simple RTT calculation based on KCP's internal RTT
        stats.rtt_avg = kcp_->rx_srtt >> 3;  // KCP stores RTT in fixed-point format
        stats.rtt_min = std::min(stats.rtt_min, stats.rtt_avg);
        stats.rtt_max = std::max(stats.rtt_max, stats.rtt_avg);
    }
    
    return stats;
}

} // namespace network
} // namespace common