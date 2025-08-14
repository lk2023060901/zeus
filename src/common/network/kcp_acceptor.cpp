#include "common/network/kcp_acceptor.h"
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
    
    // Convert endpoint to string for logging
    std::string EndpointToString(const boost::asio::ip::udp::endpoint& ep) {
        std::ostringstream oss;
        oss << ep.address().to_string() << ":" << ep.port();
        return oss.str();
    }
}

KcpAcceptor::KcpAcceptor(boost::asio::any_io_executor executor,
                        uint16_t port,
                        const std::string& bind_address,
                        const KcpConnector::KcpConfig& config)
    : executor_(executor)
    , socket_(std::make_shared<boost::asio::ip::udp::socket>(executor))
    , port_(port)
    , bind_address_(bind_address)
    , running_(false)
    , default_config_(config)
    , connection_counter_(0)
    , max_connections_(1000)
    , connection_timeout_ms_(300000)  // 5 minutes
    , next_conv_id_(1)
    , update_timer_(std::make_unique<boost::asio::steady_timer>(executor))
    , update_interval_ms_(10)
    , cleanup_timer_(std::make_unique<boost::asio::steady_timer>(executor)) {
    
    NETWORK_LOG_INFO("KcpAcceptor created for {}:{}", bind_address_, port_);
}

KcpAcceptor::~KcpAcceptor() {
    Stop();
    NETWORK_LOG_INFO("KcpAcceptor destroyed");
}

bool KcpAcceptor::Start(connection_handler_type connection_handler) {
    if (running_) {
        NETWORK_LOG_WARN("KcpAcceptor already running");
        return false;
    }
    
    connection_handler_ = std::move(connection_handler);
    
    try {
        // Create and bind socket
        boost::asio::ip::udp::endpoint bind_endpoint(
            boost::asio::ip::make_address(bind_address_), port_);
        
        socket_->open(bind_endpoint.protocol());
        socket_->set_option(boost::asio::ip::udp::socket::reuse_address(true));
        
        // Set socket buffer sizes for better performance
        socket_->set_option(boost::asio::ip::udp::socket::send_buffer_size(64 * 1024));
        socket_->set_option(boost::asio::ip::udp::socket::receive_buffer_size(64 * 1024));
        
        socket_->bind(bind_endpoint);
        
        running_ = true;
        
        NETWORK_LOG_INFO("KcpAcceptor listening on {}", GetListeningEndpoint());
        
        // Start receiving packets
        StartReceive();
        
        // Start KCP update timer
        StartUpdateTimer();
        
        // Start cleanup timer
        cleanup_timer_->expires_after(std::chrono::seconds(30));
        cleanup_timer_->async_wait([this](boost::system::error_code ec) {
            if (!ec && running_) {
                CleanupConnections();
            }
        });
        
        return true;
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to start KcpAcceptor: {}", e.what());
        running_ = false;
        return false;
    }
}

void KcpAcceptor::Stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    NETWORK_LOG_INFO("Stopping KcpAcceptor");
    
    // Cancel timers
    if (update_timer_) {
        update_timer_->cancel();
    }
    
    if (cleanup_timer_) {
        cleanup_timer_->cancel();
    }
    
    // Close socket
    if (socket_) {
        boost::system::error_code ec;
        socket_->close(ec);
        if (ec) {
            NETWORK_LOG_WARN("Error closing KcpAcceptor socket: {}", ec.message());
        }
    }
    
    // Clear connections
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_by_conv_.clear();
        connections_by_endpoint_.clear();
    }
    
    NETWORK_LOG_INFO("KcpAcceptor stopped");
}

std::string KcpAcceptor::GetListeningEndpoint() const {
    if (socket_ && socket_->is_open()) {
        boost::system::error_code ec;
        auto endpoint = socket_->local_endpoint(ec);
        if (!ec) {
            return EndpointToString(endpoint);
        }
    }
    return "not_listening";
}

size_t KcpAcceptor::GetConnectionCount() const {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    return connections_by_conv_.size();
}

void KcpAcceptor::SetMaxConnections(size_t max_connections) {
    max_connections_ = max_connections;
    NETWORK_LOG_INFO("KcpAcceptor max connections set to {}", max_connections);
}

void KcpAcceptor::SetConnectionTimeout(uint32_t timeout_ms) {
    connection_timeout_ms_ = timeout_ms;
    NETWORK_LOG_INFO("KcpAcceptor connection timeout set to {}ms", timeout_ms);
}

void KcpAcceptor::SetUpdateInterval(uint32_t interval_ms) {
    update_interval_ms_ = interval_ms;
    NETWORK_LOG_INFO("KcpAcceptor update interval set to {}ms", interval_ms);
}

void KcpAcceptor::StartReceive() {
    if (!running_ || !socket_ || !socket_->is_open()) {
        return;
    }
    
    socket_->async_receive_from(
        boost::asio::buffer(receive_buffer_),
        sender_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_transferred) {
            HandleReceive(ec, bytes_transferred, sender_endpoint_);
        });
}

void KcpAcceptor::HandleReceive(boost::system::error_code ec, size_t bytes_transferred,
                               const boost::asio::ip::udp::endpoint& sender_endpoint) {
    if (ec) {
        if (ec != boost::asio::error::operation_aborted && running_) {
            NETWORK_LOG_ERROR("KcpAcceptor receive error: {}", ec.message());
        }
        
        // Continue receiving if still running
        if (running_) {
            StartReceive();
        }
        return;
    }
    
    if (bytes_transferred == 0) {
        StartReceive();
        return;
    }
    
    NETWORK_LOG_TRACE("KcpAcceptor received {} bytes from {}", 
                     bytes_transferred, EndpointToString(sender_endpoint));
    
    // Copy received data
    std::vector<uint8_t> data(receive_buffer_.begin(), 
                             receive_buffer_.begin() + bytes_transferred);
    
    // Check if this is a handshake packet
    if (bytes_transferred >= sizeof(HandshakePacket)) {
        ProcessHandshakePacket(data, sender_endpoint);
    } else {
        // Try to process as KCP packet for existing connections
        ProcessKcpPacket(data, sender_endpoint);
    }
    
    // Continue receiving
    StartReceive();
}

void KcpAcceptor::ProcessHandshakePacket(const std::vector<uint8_t>& data, 
                                        const boost::asio::ip::udp::endpoint& sender) {
    if (data.size() < sizeof(HandshakePacket)) {
        return;
    }
    
    HandshakePacket handshake;
    std::memcpy(&handshake, data.data(), sizeof(handshake));
    
    // Check magic and type
    if (handshake.magic == HANDSHAKE_MAGIC && handshake.type == HANDSHAKE_REQUEST) {
        // Check connection limits
        if (max_connections_ > 0 && GetConnectionCount() >= max_connections_) {
            NETWORK_LOG_WARN("KcpAcceptor rejecting connection from {} - max connections reached ({})", 
                           EndpointToString(sender), max_connections_);
            return;
        }
        
        // Check if connection already exists for this endpoint
        auto existing_conn = FindConnectionByEndpoint(sender);
        if (existing_conn) {
            NETWORK_LOG_DEBUG("Connection from {} already exists", EndpointToString(sender));
            return;
        }
        
        // Create new connection
        auto connection = CreateConnection(sender);
        if (!connection) {
            NETWORK_LOG_ERROR("Failed to create connection for {}", EndpointToString(sender));
            return;
        }
        
        // Send handshake response
        HandshakePacket response;
        response.magic = HANDSHAKE_MAGIC;
        response.type = HANDSHAKE_RESPONSE;
        response.conv_id = connection->GetConfig().conv_id;
        
        std::vector<uint8_t> response_data(sizeof(response));
        std::memcpy(response_data.data(), &response, sizeof(response));
        
        socket_->async_send_to(boost::asio::buffer(response_data), sender,
            [this, sender, connection](boost::system::error_code ec, std::size_t) {
                if (ec) {
                    NETWORK_LOG_ERROR("Failed to send handshake response to {}: {}", 
                                    EndpointToString(sender), ec.message());
                    return;
                }
                
                NETWORK_LOG_INFO("KcpAcceptor accepted connection from {} with conv_id {}", 
                               EndpointToString(sender), connection->GetConfig().conv_id);
                
                // Notify handler
                if (connection_handler_) {
                    connection_handler_(connection);
                }
            });
    }
}

void KcpAcceptor::ProcessKcpPacket(const std::vector<uint8_t>& data,
                                  const boost::asio::ip::udp::endpoint& sender) {
    // Find connection by endpoint first
    auto connection = FindConnectionByEndpoint(sender);
    if (!connection) {
        NETWORK_LOG_DEBUG("Received KCP packet from unknown endpoint: {}", EndpointToString(sender));
        return;
    }
    
    // Forward packet to connection for processing
    connection->Input(data);
}

std::shared_ptr<KcpConnector> KcpAcceptor::FindConnection(uint32_t conv_id) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_by_conv_.find(conv_id);
    return (it != connections_by_conv_.end()) ? it->second : nullptr;
}

std::shared_ptr<KcpConnector> KcpAcceptor::FindConnectionByEndpoint(const boost::asio::ip::udp::endpoint& endpoint) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    std::string endpoint_key = EndpointToString(endpoint);
    auto it = connections_by_endpoint_.find(endpoint_key);
    return (it != connections_by_endpoint_.end()) ? it->second : nullptr;
}

std::shared_ptr<KcpConnector> KcpAcceptor::CreateConnection(const boost::asio::ip::udp::endpoint& endpoint) {
    std::string connection_id = GenerateConnectionId(endpoint);
    uint32_t conv_id = AllocateConversationId();
    
    // Create configuration for this connection
    auto config = default_config_;
    config.conv_id = conv_id;
    
    // Create the connection (server-side constructor)
    auto connection = std::make_shared<KcpConnector>(socket_, endpoint, connection_id, config);
    
    // Store connection in both maps
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_by_conv_[conv_id] = connection;
        connections_by_endpoint_[EndpointToString(endpoint)] = connection;
        connection_counter_++;
    }
    
    NETWORK_LOG_DEBUG("Created KcpConnector for {} with conv_id {}", 
                     EndpointToString(endpoint), conv_id);
    
    return connection;
}

void KcpAcceptor::CleanupConnections() {
    if (!running_) {
        return;
    }
    
    std::vector<uint32_t> conv_ids_to_remove;
    std::vector<std::string> endpoints_to_remove;
    
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        
        // Find disconnected connections
        for (auto it = connections_by_conv_.begin(); it != connections_by_conv_.end();) {
            auto& connection = it->second;
            if (!connection || !connection->IsConnected()) {
                NETWORK_LOG_DEBUG("Cleaning up disconnected connection: conv_id {}", it->first);
                
                conv_ids_to_remove.push_back(it->first);
                
                // Find corresponding endpoint key
                auto endpoint_key = connection ? connection->GetRemoteEndpoint() : "unknown";
                endpoints_to_remove.push_back(endpoint_key);
                
                it = connections_by_conv_.erase(it);
            } else {
                ++it;
            }
        }
        
        // Remove from endpoint map
        for (const auto& endpoint_key : endpoints_to_remove) {
            connections_by_endpoint_.erase(endpoint_key);
        }
    }
    
    // Deallocate conversation IDs
    for (uint32_t conv_id : conv_ids_to_remove) {
        DeallocateConversationId(conv_id);
    }
    
    if (!conv_ids_to_remove.empty()) {
        NETWORK_LOG_DEBUG("Cleaned up {} disconnected connections", conv_ids_to_remove.size());
    }
    
    // Schedule next cleanup
    if (cleanup_timer_ && running_) {
        cleanup_timer_->expires_after(std::chrono::seconds(30));
        cleanup_timer_->async_wait([this](boost::system::error_code ec) {
            if (!ec && running_) {
                CleanupConnections();
            }
        });
    }
}

std::string KcpAcceptor::GenerateConnectionId(const boost::asio::ip::udp::endpoint& endpoint) {
    std::ostringstream oss;
    oss << "kcp_" << endpoint.address().to_string() << "_" << endpoint.port() 
        << "_" << connection_counter_.load();
    return oss.str();
}

uint32_t KcpAcceptor::AllocateConversationId() {
    std::lock_guard<std::mutex> lock(conv_id_mutex_);
    
    uint32_t conv_id = next_conv_id_.fetch_add(1);
    
    // Ensure conv_id is not 0 and not already allocated
    while (conv_id == 0 || allocated_conv_ids_.find(conv_id) != allocated_conv_ids_.end()) {
        conv_id = next_conv_id_.fetch_add(1);
    }
    
    allocated_conv_ids_.insert(conv_id);
    
    NETWORK_LOG_DEBUG("Allocated conversation ID: {}", conv_id);
    return conv_id;
}

void KcpAcceptor::DeallocateConversationId(uint32_t conv_id) {
    std::lock_guard<std::mutex> lock(conv_id_mutex_);
    
    auto it = allocated_conv_ids_.find(conv_id);
    if (it != allocated_conv_ids_.end()) {
        allocated_conv_ids_.erase(it);
        NETWORK_LOG_DEBUG("Deallocated conversation ID: {}", conv_id);
    }
}

void KcpAcceptor::StartUpdateTimer() {
    if (!running_ || !update_timer_) {
        return;
    }
    
    update_timer_->expires_after(std::chrono::milliseconds(update_interval_ms_));
    update_timer_->async_wait([this](boost::system::error_code ec) {
        HandleUpdateTimer(ec);
    });
}

void KcpAcceptor::HandleUpdateTimer(boost::system::error_code ec) {
    if (ec == boost::asio::error::operation_aborted) {
        return; // Timer was cancelled
    }
    
    if (ec) {
        NETWORK_LOG_WARN("KcpAcceptor update timer error: {}", ec.message());
        return;
    }
    
    if (!running_) {
        return;
    }
    
    // Update all KCP connections
    uint32_t current_time = GetCurrentTimeMs();
    
    std::vector<std::shared_ptr<KcpConnector>> connections_to_update;
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_to_update.reserve(connections_by_conv_.size());
        
        for (const auto& pair : connections_by_conv_) {
            if (pair.second) {
                connections_to_update.push_back(pair.second);
            }
        }
    }
    
    // Update connections outside of lock to avoid deadlock
    for (auto& connection : connections_to_update) {
        if (connection && connection->IsConnected()) {
            connection->Update(current_time);
        }
    }
    
    // Continue update timer
    if (running_) {
        StartUpdateTimer();
    }
}

} // namespace network
} // namespace common