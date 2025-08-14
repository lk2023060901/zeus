#pragma once

/**
 * @file zeus_network.h
 * @brief Zeus Network Module - Main include header
 * 
 * This header provides easy access to all Zeus network functionality including:
 * - TCP connections and server
 * - KCP reliable UDP connections and server
 * - Network event system with hooks
 * - Comprehensive network logging
 * - High-performance async I/O based on Boost.ASIO
 * 
 * Usage:
 * ```cpp
 * #include "common/network/zeus_network.h"
 * using namespace common::network;
 * 
 * // Initialize network logging
 * NetworkLogger::Instance().Initialize("network");
 * 
 * // Create TCP connection
 * auto tcp_conn = std::make_shared<TcpConnector>(executor, "client1");
 * tcp_conn->AsyncConnect("127.0.0.1:8080", [](boost::system::error_code ec) {
 *     if (!ec) {
 *         // Connected successfully
 *     }
 * });
 * 
 * // Register network event hooks
 * REGISTER_NETWORK_HOOK({NetworkEventType::CONNECTION_ESTABLISHED}, 
 *                       "connection_logger", 
 *                       [](const NetworkEvent& event) {
 *                           std::cout << "Connection established: " << event.connection_id << std::endl;
 *                       });
 * ```
 */

// Core network components
#include "network_logger.h"
#include "network_events.h"
#include "connection.h"

// Protocol implementations  
#include "tcp_connector.h"
#include "kcp_connector.h"

// Server implementations
#include "tcp_acceptor.h"
#include "kcp_acceptor.h"

// HTTP module
#include "http/http_common.h"
#include "http/http_message.h"
#include "http/http_client.h"
#include "http/http_server.h"
#include "http/http_router.h"
#include "http/http_middleware.h"

namespace common {
namespace network {

/**
 * @brief Zeus Network Module Version Information
 */
struct NetworkVersion {
    static constexpr int MAJOR = 1;
    static constexpr int MINOR = 0; 
    static constexpr int PATCH = 0;
    static constexpr const char* VERSION_STRING = "1.0.0";
};

/**
 * @brief Network module initialization and cleanup utilities
 */
class NetworkModule {
public:
    /**
     * @brief Initialize the entire network module
     * @param config_file Network logging configuration file (optional)
     * @param enable_logging Whether to enable network logging
     * @return true if initialization successful
     */
    static bool Initialize(const std::string& config_file = "", bool enable_logging = true);
    
    /**
     * @brief Shutdown the network module and cleanup resources
     */
    static void Shutdown();
    
    /**
     * @brief Check if module is initialized
     */
    static bool IsInitialized();
    
    /**
     * @brief Get module version
     */
    static const char* GetVersion() { return NetworkVersion::VERSION_STRING; }
    
    /**
     * @brief Get module statistics
     */
    struct ModuleStats {
        size_t active_tcp_connections = 0;
        size_t active_kcp_connections = 0;
        size_t total_registered_hooks = 0;
        uint64_t total_bytes_sent = 0;
        uint64_t total_bytes_received = 0;
        std::chrono::steady_clock::time_point initialized_at;
    };
    
    /**
     * @brief Get current module statistics
     */
    static ModuleStats GetStats();

private:
    static std::atomic<bool> initialized_;
    static std::chrono::steady_clock::time_point init_time_;
    static ModuleStats stats_;
};

/**
 * @brief Utility functions for common network operations
 */
namespace NetworkUtils {
    
    /**
     * @brief Generate a unique connection ID
     * @param prefix Optional prefix for the ID
     * @return Unique connection identifier
     */
    std::string GenerateConnectionId(const std::string& prefix = "conn");
    
    /**
     * @brief Parse endpoint string into host and port
     * @param endpoint Endpoint string (host:port)
     * @return Pair of (host, port) or empty strings if invalid
     */
    std::pair<std::string, std::string> ParseEndpoint(const std::string& endpoint);
    
    /**
     * @brief Validate endpoint format
     * @param endpoint Endpoint string to validate
     * @return true if valid format
     */
    bool IsValidEndpoint(const std::string& endpoint);
    
    /**
     * @brief Get local IP addresses
     * @return Vector of local IP address strings
     */
    std::vector<std::string> GetLocalIpAddresses();
    
    /**
     * @brief Check if port is available for binding
     * @param port Port number to check
     * @param protocol Protocol type ("tcp" or "udp")
     * @param bind_address Address to bind to (default: "0.0.0.0")
     * @return true if port is available
     */
    bool IsPortAvailable(uint16_t port, const std::string& protocol = "tcp", 
                        const std::string& bind_address = "0.0.0.0");
    
    /**
     * @brief Find an available port in a given range
     * @param start_port Starting port number
     * @param end_port Ending port number
     * @param protocol Protocol type ("tcp" or "udp")
     * @param bind_address Address to bind to (default: "0.0.0.0")
     * @return Available port number, or 0 if none found
     */
    uint16_t FindAvailablePort(uint16_t start_port, uint16_t end_port, 
                              const std::string& protocol = "tcp",
                              const std::string& bind_address = "0.0.0.0");
    
    /**
     * @brief Convert bytes to human readable string
     * @param bytes Number of bytes
     * @return Human readable string (e.g., "1.5 MB")
     */
    std::string BytesToString(uint64_t bytes);
    
    /**
     * @brief Convert duration to human readable string
     * @param duration Duration in milliseconds
     * @return Human readable string (e.g., "1m 30s")
     */
    std::string DurationToString(uint64_t duration_ms);
}

/**
 * @brief Predefined network event hook functions for common use cases
 */
namespace CommonHooks {
    
    /**
     * @brief Hook that logs all network events to console
     */
    void ConsoleLogger(const NetworkEvent& event);
    
    /**
     * @brief Hook that tracks connection statistics
     */
    void ConnectionStatsTracker(const NetworkEvent& event);
    
    /**
     * @brief Hook that monitors connection health and logs warnings
     */
    void ConnectionHealthMonitor(const NetworkEvent& event);
    
    /**
     * @brief Hook that implements basic rate limiting
     * @param max_connections Maximum connections per IP
     * @param time_window_ms Time window for rate limiting in milliseconds
     * @return Hook function that implements rate limiting
     */
    NetworkEventHook CreateRateLimitHook(size_t max_connections, uint32_t time_window_ms);
    
    /**
     * @brief Hook that implements connection timeout monitoring
     * @param timeout_ms Timeout in milliseconds
     * @return Hook function that monitors timeouts
     */
    NetworkEventHook CreateTimeoutMonitorHook(uint32_t timeout_ms);
    
    /**
     * @brief Hook that saves connection events to a file
     * @param filename File to save events to
     * @return Hook function that logs to file
     */
    NetworkEventHook CreateFileLoggerHook(const std::string& filename);
}

/**
 * @brief High-level network factory functions
 */
namespace NetworkFactory {
    
    /**
     * @brief Create a configured TCP client connection
     * @param executor Boost.ASIO executor
     * @param connection_id Connection identifier
     * @param enable_keepalive Whether to enable TCP keepalive
     * @param enable_nodelay Whether to enable TCP_NODELAY
     * @return Shared pointer to TcpConnector
     */
    std::shared_ptr<TcpConnector> CreateTcpClient(
        boost::asio::any_io_executor executor,
        const std::string& connection_id = "",
        bool enable_keepalive = true,
        bool enable_nodelay = true
    );
    
    /**
     * @brief Create a configured TCP server
     * @param executor Boost.ASIO executor
     * @param port Port to listen on
     * @param bind_address Address to bind to
     * @param max_connections Maximum concurrent connections
     * @return Shared pointer to TcpAcceptor
     */
    std::shared_ptr<TcpAcceptor> CreateTcpServer(
        boost::asio::any_io_executor executor,
        uint16_t port,
        const std::string& bind_address = "0.0.0.0",
        size_t max_connections = 1000
    );
    
    /**
     * @brief Create a configured KCP client connection
     * @param executor Boost.ASIO executor
     * @param connection_id Connection identifier
     * @param config KCP configuration
     * @return Shared pointer to KcpConnector
     */
    std::shared_ptr<KcpConnector> CreateKcpClient(
        boost::asio::any_io_executor executor,
        const std::string& connection_id = "",
        const KcpConnector::KcpConfig& config = KcpConnector::KcpConfig{}
    );
    
    /**
     * @brief Create a configured KCP server
     * @param executor Boost.ASIO executor
     * @param port Port to listen on
     * @param bind_address Address to bind to
     * @param config Default KCP configuration
     * @param max_connections Maximum concurrent connections
     * @return Shared pointer to KcpAcceptor
     */
    std::shared_ptr<KcpAcceptor> CreateKcpServer(
        boost::asio::any_io_executor executor,
        uint16_t port,
        const std::string& bind_address = "0.0.0.0",
        const KcpConnector::KcpConfig& config = KcpConnector::KcpConfig{},
        size_t max_connections = 1000
    );
}

} // namespace network
} // namespace common

// Convenience macros for easy module usage
#define ZEUS_NETWORK_INIT(config_file) \
    common::network::NetworkModule::Initialize(config_file, true)

#define ZEUS_NETWORK_SHUTDOWN() \
    common::network::NetworkModule::Shutdown()

#define ZEUS_NETWORK_LOG_INFO(msg, ...) \
    NETWORK_LOG_INFO(msg, ##__VA_ARGS__)

#define ZEUS_NETWORK_LOG_ERROR(msg, ...) \
    NETWORK_LOG_ERROR(msg, ##__VA_ARGS__)

#define ZEUS_NETWORK_HOOK_CONSOLE_LOGGER() \
    REGISTER_GLOBAL_NETWORK_HOOK("console_logger", common::network::CommonHooks::ConsoleLogger)

#define ZEUS_NETWORK_HOOK_STATS_TRACKER() \
    REGISTER_GLOBAL_NETWORK_HOOK("stats_tracker", common::network::CommonHooks::ConnectionStatsTracker)