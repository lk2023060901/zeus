#pragma once

#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <boost/asio.hpp>
#include "kcp_connector.h"

namespace common {
namespace network {

/**
 * @brief KCP Acceptor for handling multiple KCP connections over a single UDP socket
 * 
 * 基于真实的skywind3000/kcp库实现，提供：
 * - UDP多路复用的KCP会话管理
 * - 自动conversation ID分配
 * - 连接超时和清理
 * - 高性能的KCP状态更新
 */
class KcpAcceptor {
public:
    using connection_handler_type = std::function<void(std::shared_ptr<KcpConnector>)>;

    /**
     * @brief Constructor
     * @param executor Boost.ASIO executor
     * @param port Port to listen on
     * @param bind_address Address to bind to (default: all interfaces)
     * @param config Default KCP configuration for new connections
     */
    explicit KcpAcceptor(boost::asio::any_io_executor executor,
                        uint16_t port,
                        const std::string& bind_address = "0.0.0.0",
                        const KcpConnector::KcpConfig& config = KcpConnector::KcpConfig{});

    /**
     * @brief Destructor
     */
    ~KcpAcceptor();

    /**
     * @brief Start the acceptor
     * @param connection_handler Handler for new connections
     * @return true if started successfully
     */
    bool Start(connection_handler_type connection_handler);

    /**
     * @brief Stop the acceptor
     */
    void Stop();

    /**
     * @brief Check if acceptor is running
     */
    bool IsRunning() const { return running_; }

    /**
     * @brief Get listening endpoint
     */
    std::string GetListeningEndpoint() const;

    /**
     * @brief Get current connection count
     */
    size_t GetConnectionCount() const;

    /**
     * @brief Set maximum number of concurrent connections
     */
    void SetMaxConnections(size_t max_connections);

    /**
     * @brief Set connection timeout for idle connections
     */
    void SetConnectionTimeout(uint32_t timeout_ms);

    /**
     * @brief Set KCP update interval in milliseconds
     */
    void SetUpdateInterval(uint32_t interval_ms);

private:
    void StartReceive();
    void HandleReceive(boost::system::error_code ec, size_t bytes_transferred,
                      const boost::asio::ip::udp::endpoint& sender_endpoint);
    
    void ProcessHandshakePacket(const std::vector<uint8_t>& data, 
                               const boost::asio::ip::udp::endpoint& sender);
    
    void ProcessKcpPacket(const std::vector<uint8_t>& data,
                         const boost::asio::ip::udp::endpoint& sender);
    
    std::shared_ptr<KcpConnector> FindConnection(uint32_t conv_id);
    std::shared_ptr<KcpConnector> FindConnectionByEndpoint(const boost::asio::ip::udp::endpoint& endpoint);
    
    std::shared_ptr<KcpConnector> CreateConnection(const boost::asio::ip::udp::endpoint& endpoint);
    
    void CleanupConnections();
    std::string GenerateConnectionId(const boost::asio::ip::udp::endpoint& endpoint);
    
    uint32_t AllocateConversationId();
    void DeallocateConversationId(uint32_t conv_id);
    
    void StartUpdateTimer();
    void HandleUpdateTimer(boost::system::error_code ec);
    
    // Network components
    boost::asio::any_io_executor executor_;
    std::shared_ptr<boost::asio::ip::udp::socket> socket_;
    
    uint16_t port_;
    std::string bind_address_;
    bool running_ = false;
    
    // Configuration
    KcpConnector::KcpConfig default_config_;
    connection_handler_type connection_handler_;
    
    // Connection management
    std::unordered_map<uint32_t, std::shared_ptr<KcpConnector>> connections_by_conv_;
    std::unordered_map<std::string, std::shared_ptr<KcpConnector>> connections_by_endpoint_;
    mutable std::mutex connections_mutex_;
    
    std::atomic<size_t> connection_counter_{0};
    size_t max_connections_ = 1000;
    uint32_t connection_timeout_ms_ = 300000; // 5 minutes
    
    // Conversation ID management
    std::atomic<uint32_t> next_conv_id_{1};
    std::unordered_set<uint32_t> allocated_conv_ids_;
    std::mutex conv_id_mutex_;
    
    // KCP update timer
    std::unique_ptr<boost::asio::steady_timer> update_timer_;
    uint32_t update_interval_ms_ = 10; // 10ms update interval
    
    // Receive buffer
    static const size_t RECEIVE_BUFFER_SIZE = 2048;
    std::array<uint8_t, RECEIVE_BUFFER_SIZE> receive_buffer_;
    boost::asio::ip::udp::endpoint sender_endpoint_;
    
    // Cleanup timer
    std::unique_ptr<boost::asio::steady_timer> cleanup_timer_;
    
    // Protocol constants
    static constexpr uint32_t HANDSHAKE_MAGIC = 0x12345678;
    static constexpr uint8_t HANDSHAKE_REQUEST = 0x01;
    static constexpr uint8_t HANDSHAKE_RESPONSE = 0x02;
    
    struct HandshakePacket {
        uint32_t magic;
        uint8_t type;
        uint32_t conv_id; // 0 for request, allocated for response
        uint8_t padding[3];
    } __attribute__((packed));
};

} // namespace network
} // namespace common