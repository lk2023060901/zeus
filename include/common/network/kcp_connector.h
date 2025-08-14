#pragma once

#include "connection.h"
#include <boost/asio/ip/udp.hpp>
#include <unordered_map>
#include <chrono>
#include <memory>

// KCP library integration
extern "C" {
#include <ikcp.h>
}

namespace common {
namespace network {

/**
 * @brief KCP Connector implementation using skywind3000/kcp
 * 
 * 基于真实的KCP库实现，提供：
 * - 低延迟可靠UDP传输
 * - 快速重传和拥塞控制
 * - 自动重连和心跳检测
 * - 与Boost.ASIO完全集成
 */
class KcpConnector : public Connection {
    friend class KcpAcceptor;  // Allow KcpAcceptor access to private members
    
public:
    using socket_type = boost::asio::ip::udp::socket;
    using endpoint_type = boost::asio::ip::udp::endpoint;
    using resolver_type = boost::asio::ip::udp::resolver;

    /**
     * @brief KCP configuration parameters
     */
    struct KcpConfig {
        uint32_t conv_id = 0;           // Conversation ID (must be same for both ends)
        int nodelay = 1;                // Enable nodelay mode (0=disable, 1=enable)
        int interval = 10;              // Update interval (ms)
        int resend = 2;                 // Fast resend mode (0=off, 1=on, 2=2ACKs trigger)
        int nc = 1;                     // Disable congestion control (0=enable, 1=disable)
        int sndwnd = 128;               // Send window size
        int rcvwnd = 128;               // Receive window size
        int mtu = 1400;                 // Maximum transmission unit
        uint32_t timeout_ms = 10000;    // Connection timeout
        bool enable_crc32 = true;       // Enable CRC32 checksum
        uint32_t heartbeat_interval = 30000; // Heartbeat interval in ms
        
        KcpConfig() = default;
        
        static KcpConfig Default() {
            return KcpConfig{};
        }
        
        static KcpConfig FastMode() {
            KcpConfig config;
            config.nodelay = 1;
            config.interval = 10;
            config.resend = 2;
            config.nc = 1;
            return config;
        }
        
        static KcpConfig NormalMode() {
            KcpConfig config;
            config.nodelay = 0;
            config.interval = 40;
            config.resend = 0;
            config.nc = 0;
            return config;
        }
    };

    /**
     * @brief Constructor for client connections
     * @param executor Boost.ASIO executor
     * @param connection_id Unique connection identifier
     * @param config KCP configuration
     */
    explicit KcpConnector(boost::asio::any_io_executor executor, 
                         const std::string& connection_id,
                         const KcpConfig& config = KcpConfig::Default());

    /**
     * @brief Constructor for acceptor-side connections
     * @param socket UDP socket (shared with acceptor)
     * @param endpoint Remote endpoint
     * @param connection_id Unique connection identifier
     * @param config KCP configuration
     */
    explicit KcpConnector(std::shared_ptr<socket_type> socket,
                         const endpoint_type& endpoint,
                         const std::string& connection_id,
                         const KcpConfig& config = KcpConfig::Default());

    /**
     * @brief Destructor
     */
    ~KcpConnector() override;

    // Connection interface implementation
    std::string GetProtocol() const override { return "KCP"; }
    std::string GetRemoteEndpoint() const override;
    std::string GetLocalEndpoint() const override;

    void AsyncConnect(const std::string& endpoint, 
                     std::function<void(boost::system::error_code)> callback) override;

    void AsyncSend(const std::vector<uint8_t>& data, SendCallback callback = nullptr) override;

    void Close() override;
    void ForceClose() override;

    /**
     * @brief Get KCP configuration
     */
    const KcpConfig& GetConfig() const { return config_; }

    /**
     * @brief Get KCP statistics
     */
    struct KcpStats {
        // KCP internal statistics
        uint32_t snd_una = 0;      // First unacknowledged packet
        uint32_t snd_nxt = 0;      // Next packet to be sent
        uint32_t rcv_nxt = 0;      // Next expected packet
        uint32_t ssthresh = 0;     // Slow start threshold
        uint32_t rto = 0;          // Retransmission timeout
        uint32_t cwnd = 0;         // Congestion window
        uint32_t probe = 0;        // Probe flag
        uint32_t current = 0;      // Current timestamp
        uint32_t interval = 0;     // Update interval
        uint32_t ts_flush = 0;     // Next flush timestamp
        uint32_t nsnd_buf = 0;     // Send buffer count
        uint32_t nrcv_buf = 0;     // Receive buffer count
        uint32_t nrcv_que = 0;     // Receive queue count
        uint32_t nsnd_que = 0;     // Send queue count
        
        // Connection-level statistics (for test compatibility)
        uint64_t packets_sent = 0;     // Total packets sent
        uint64_t packets_received = 0; // Total packets received  
        uint64_t bytes_sent = 0;       // Total bytes sent
        uint64_t bytes_received = 0;   // Total bytes received
        uint32_t rtt_avg = 0;          // Average RTT in ms
        uint32_t rtt_min = UINT32_MAX; // Minimum RTT in ms
        uint32_t rtt_max = 0;          // Maximum RTT in ms
    };
    
    /**
     * @brief Get current KCP statistics
     */
    KcpStats GetKcpStats() const;
    
    /**
     * @brief Update KCP state (called periodically by acceptor)
     */
    void Update(uint32_t current_time_ms);
    
    /**
     * @brief Input data from UDP socket into KCP
     */
    void Input(const std::vector<uint8_t>& data);
    
    /**
     * @brief Check if need to call Update()
     */
    uint32_t Check(uint32_t current_time_ms) const;

protected:
    void SendHeartbeat() override;

private:
    // KCP callbacks (must be static for C interface)
    static int KcpOutputCallback(const char* buf, int len, ikcpcb* kcp, void* user);
    
    // Internal methods
    void InitializeKcp();
    void DestroyKcp();
    void ConfigureKcp();
    
    void StartReceiveLoop();
    void HandleUdpReceive(boost::system::error_code ec, size_t bytes_transferred);
    
    void ProcessKcpData();
    void HandleKcpOutput(const std::vector<uint8_t>& data);
    
    void DoConnect(const std::string& host, const std::string& port,
                   std::function<void(boost::system::error_code)> callback);
    
    void HandleResolve(boost::system::error_code ec, resolver_type::results_type endpoints,
                      std::function<void(boost::system::error_code)> callback);
                      
    void HandleConnect(boost::system::error_code ec, 
                      std::function<void(boost::system::error_code)> callback);
    
    void SendHandshake();
    void HandleHandshakeResponse(const std::vector<uint8_t>& data);
    
    void StartHeartbeatTimer();
    void HandleHeartbeatTimer(boost::system::error_code ec);
    
    void CloseSocket();
    std::string EndpointToString(const endpoint_type& ep) const;
    
    // KCP instance
    ikcpcb* kcp_ = nullptr;
    KcpConfig config_;
    
    // Network components  
    std::shared_ptr<socket_type> socket_;
    std::unique_ptr<resolver_type> resolver_;
    endpoint_type remote_endpoint_;
    endpoint_type local_endpoint_;
    
    // Connection state
    bool socket_owned_ = true; // Whether we own the socket (client) or share it (acceptor-side)
    bool connected_ = false;
    bool connecting_ = false;
    
    // Receive buffer for UDP
    static const size_t UDP_RECEIVE_BUFFER_SIZE = 2048;
    std::array<uint8_t, UDP_RECEIVE_BUFFER_SIZE> udp_receive_buffer_;
    
    // KCP receive buffer
    std::vector<uint8_t> kcp_receive_buffer_;
    
    // Heartbeat timer
    std::unique_ptr<boost::asio::steady_timer> heartbeat_timer_;
    
    // Timestamps
    std::chrono::steady_clock::time_point last_update_time_;
    std::chrono::steady_clock::time_point connection_start_time_;
    
    // Output callback data queue
    std::vector<std::vector<uint8_t>> output_queue_;
    std::mutex output_mutex_;
    
    // Handshake state
    bool handshake_sent_ = false;
    bool handshake_completed_ = false;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    uint64_t bytes_sent_ = 0;
    uint64_t bytes_received_ = 0;
    uint64_t packets_sent_ = 0;
    uint64_t packets_received_ = 0;
};

} // namespace network
} // namespace common