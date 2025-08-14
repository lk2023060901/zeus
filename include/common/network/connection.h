#pragma once

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <functional>
#include <boost/asio.hpp>
#include "network_events.h"

namespace common {
namespace network {

/**
 * @brief Connection state enumeration
 */
enum class ConnectionState {
    DISCONNECTED,   // Not connected
    CONNECTING,     // Connection in progress
    CONNECTED,      // Successfully connected
    DISCONNECTING,  // Disconnection in progress
    ERROR          // Connection error state
};

/**
 * @brief Connection statistics
 */
struct ConnectionStats {
    std::chrono::steady_clock::time_point connected_at;
    std::chrono::steady_clock::time_point last_activity;
    
    std::atomic<uint64_t> bytes_sent{0};
    std::atomic<uint64_t> bytes_received{0};
    std::atomic<uint64_t> messages_sent{0};
    std::atomic<uint64_t> messages_received{0};
    std::atomic<uint32_t> errors_count{0};
    
    // Latency tracking (for protocols that support it)
    std::atomic<uint32_t> last_ping_ms{0};
    std::atomic<uint32_t> avg_ping_ms{0};
    
    ConnectionStats() {
        auto now = std::chrono::steady_clock::now();
        connected_at = now;
        last_activity = now;
    }
};

/**
 * @brief Abstract base class for all network connections
 */
class Connection : public std::enable_shared_from_this<Connection> {
public:
    using SendCallback = std::function<void(boost::system::error_code, size_t)>;
    using DataHandler = std::function<void(const std::vector<uint8_t>&)>;
    using ErrorHandler = std::function<void(boost::system::error_code)>;
    using StateChangeHandler = std::function<void(ConnectionState, ConnectionState)>;

    /**
     * @brief Constructor
     * @param executor Boost.ASIO executor for async operations
     * @param connection_id Unique connection identifier
     */
    explicit Connection(boost::asio::any_io_executor executor, const std::string& connection_id);

    /**
     * @brief Virtual destructor
     */
    virtual ~Connection();

    /**
     * @brief Get connection ID
     */
    const std::string& GetConnectionId() const { return connection_id_; }

    /**
     * @brief Get current connection state
     */
    ConnectionState GetState() const { return state_.load(); }

    /**
     * @brief Get protocol name
     */
    virtual std::string GetProtocol() const = 0;

    /**
     * @brief Get remote endpoint information
     */
    virtual std::string GetRemoteEndpoint() const = 0;

    /**
     * @brief Get local endpoint information
     */
    virtual std::string GetLocalEndpoint() const = 0;

    /**
     * @brief Start connection (async)
     * @param endpoint Target endpoint to connect to
     * @param callback Completion callback
     */
    virtual void AsyncConnect(const std::string& endpoint, 
                            std::function<void(boost::system::error_code)> callback) = 0;

    /**
     * @brief Send data asynchronously
     * @param data Data to send
     * @param callback Send completion callback
     */
    virtual void AsyncSend(const std::vector<uint8_t>& data, SendCallback callback = nullptr) = 0;

    /**
     * @brief Send data asynchronously (string version)
     * @param data String data to send
     * @param callback Send completion callback
     */
    void AsyncSend(const std::string& data, SendCallback callback = nullptr);

    /**
     * @brief Close connection gracefully
     */
    virtual void Close() = 0;

    /**
     * @brief Force close connection immediately
     */
    virtual void ForceClose() = 0;

    /**
     * @brief Check if connection is active
     */
    bool IsConnected() const { return state_.load() == ConnectionState::CONNECTED; }

    /**
     * @brief Get connection statistics
     */
    const ConnectionStats& GetStats() const { return stats_; }

    /**
     * @brief Set data received handler
     * @param handler Handler function for incoming data
     */
    void SetDataHandler(DataHandler handler) { data_handler_ = std::move(handler); }

    /**
     * @brief Set error handler
     * @param handler Handler function for connection errors
     */
    void SetErrorHandler(ErrorHandler handler) { error_handler_ = std::move(handler); }

    /**
     * @brief Set state change handler
     * @param handler Handler function for state changes
     */
    void SetStateChangeHandler(StateChangeHandler handler) { state_change_handler_ = std::move(handler); }

    /**
     * @brief Enable/disable automatic heartbeat (if supported by protocol)
     * @param enable Whether to enable heartbeat
     * @param interval Heartbeat interval in milliseconds
     */
    virtual void SetHeartbeat(bool enable, uint32_t interval_ms = 30000);

    /**
     * @brief Get executor for async operations
     */
    boost::asio::any_io_executor GetExecutor() { return executor_; }

    /**
     * @brief Set connection timeout
     * @param timeout_ms Timeout in milliseconds (0 = no timeout)
     */
    void SetTimeout(uint32_t timeout_ms) { timeout_ms_ = timeout_ms; }

    /**
     * @brief Get connection timeout
     */
    uint32_t GetTimeout() const { return timeout_ms_; }

protected:
    /**
     * @brief Update connection state and notify handlers
     * @param new_state New connection state
     */
    void UpdateState(ConnectionState new_state);

    /**
     * @brief Handle received data (calls user handler and fires events)
     * @param data Received data
     */
    void HandleDataReceived(const std::vector<uint8_t>& data);

    /**
     * @brief Handle connection error (calls user handler and fires events)
     * @param error Error code
     */
    void HandleError(boost::system::error_code error);

    /**
     * @brief Fire network event
     * @param event Event to fire
     */
    void FireEvent(const NetworkEvent& event);

    /**
     * @brief Update statistics
     */
    void UpdateStats();

    // Core connection data
    boost::asio::any_io_executor executor_;
    std::string connection_id_;
    std::atomic<ConnectionState> state_{ConnectionState::DISCONNECTED};
    ConnectionStats stats_;
    
    // Handlers
    DataHandler data_handler_;
    ErrorHandler error_handler_;
    StateChangeHandler state_change_handler_;
    
    // Configuration
    uint32_t timeout_ms_ = 30000; // 30 seconds default timeout
    bool heartbeat_enabled_ = false;
    uint32_t heartbeat_interval_ms_ = 30000;
    
    // Heartbeat timer (if supported)
    std::unique_ptr<boost::asio::steady_timer> heartbeat_timer_;

private:
    void StartHeartbeat();
    void StopHeartbeat();
    virtual void SendHeartbeat() {} // Override in derived classes if needed
};

} // namespace network
} // namespace common