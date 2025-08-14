#pragma once

#include <memory>
#include <string>
#include <functional>
#include <atomic>
#include <thread>
#include <boost/asio.hpp>
#include "tcp_connector.h"

namespace common {
namespace network {

/**
 * @brief TCP Acceptor for accepting incoming connections
 */
class TcpAcceptor {
public:
    using connection_factory_type = std::function<std::shared_ptr<TcpConnector>(TcpConnector::socket_type)>;
    using connection_handler_type = std::function<void(std::shared_ptr<TcpConnector>)>;

    /**
     * @brief Constructor
     * @param executor Boost.ASIO executor
     * @param port Port to listen on
     * @param bind_address Address to bind to (default: all interfaces)
     */
    explicit TcpAcceptor(boost::asio::any_io_executor executor, 
                        uint16_t port, 
                        const std::string& bind_address = "0.0.0.0");

    /**
     * @brief Destructor
     */
    ~TcpAcceptor();

    /**
     * @brief Start accepting connections
     * @param connection_handler Handler for new connections
     * @return true if started successfully
     */
    bool Start(connection_handler_type connection_handler);

    /**
     * @brief Stop accepting new connections
     */
    void Stop();

    /**
     * @brief Check if server is running
     */
    bool IsRunning() const { return running_; }

    /**
     * @brief Get listening endpoint
     */
    std::string GetListeningEndpoint() const;

    /**
     * @brief Set connection factory for custom connection types
     * @param factory Factory function that creates TcpConnector-derived objects
     */
    void SetConnectionFactory(connection_factory_type factory);

    /**
     * @brief Get current connection count
     */
    size_t GetConnectionCount() const;

    /**
     * @brief Set maximum number of concurrent connections
     * @param max_connections Maximum connections (0 = unlimited)
     */
    void SetMaxConnections(size_t max_connections);

private:
    void DoAccept();
    void HandleAccept(boost::system::error_code ec);
    std::string GenerateConnectionId();

    boost::asio::any_io_executor executor_;
    boost::asio::ip::tcp::acceptor acceptor_;
    
    uint16_t port_;
    std::string bind_address_;
    bool running_ = false;
    
    connection_handler_type connection_handler_;
    connection_factory_type connection_factory_;
    
    // Connection management
    std::atomic<size_t> connection_counter_{0};
    std::atomic<size_t> active_connections_{0};
    size_t max_connections_ = 1000; // Default limit
    
    // Accept socket for next connection
    TcpConnector::socket_type accept_socket_;
};

} // namespace network
} // namespace common