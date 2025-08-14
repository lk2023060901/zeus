#pragma once

#include "connection.h"
#include <boost/asio/ip/tcp.hpp>
#include <queue>
#include <mutex>

namespace common {
namespace network {

/**
 * @brief TCP connector implementation using Boost.ASIO
 */
class TcpConnector : public Connection {
public:
    using socket_type = boost::asio::ip::tcp::socket;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using resolver_type = boost::asio::ip::tcp::resolver;

    /**
     * @brief Constructor for client connections
     * @param executor Boost.ASIO executor
     * @param connection_id Unique connection identifier
     */
    explicit TcpConnector(boost::asio::any_io_executor executor, const std::string& connection_id);

    /**
     * @brief Constructor for server-accepted connections
     * @param socket Already connected socket
     * @param connection_id Unique connection identifier
     */
    explicit TcpConnector(socket_type socket, const std::string& connection_id);

    /**
     * @brief Destructor
     */
    ~TcpConnector() override;

    // Connection interface implementation
    std::string GetProtocol() const override { return "TCP"; }
    std::string GetRemoteEndpoint() const override;
    std::string GetLocalEndpoint() const override;

    void AsyncConnect(const std::string& endpoint, 
                     std::function<void(boost::system::error_code)> callback) override;

    void AsyncSend(const std::vector<uint8_t>& data, SendCallback callback = nullptr) override;

    void Close() override;
    void ForceClose() override;

    /**
     * @brief Get the underlying socket
     * @return Reference to the TCP socket
     */
    socket_type& GetSocket() { return socket_; }

    /**
     * @brief Set TCP-specific options
     */
    void SetNoDelay(bool enable);
    void SetKeepAlive(bool enable, uint32_t idle_time = 7200, uint32_t interval = 75, uint32_t probes = 9);
    void SetReceiveBufferSize(size_t size);
    void SetSendBufferSize(size_t size);
    void SetReuseAddress(bool enable);

    /**
     * @brief Configure SSL/TLS (if Boost.ASIO SSL is available)
     * Note: This would require additional SSL implementation
     */
    // void EnableSSL(boost::asio::ssl::context& context);

protected:
    void SendHeartbeat() override;

private:
    struct SendOperation {
        std::vector<uint8_t> data;
        SendCallback callback;
        std::chrono::steady_clock::time_point timestamp;
        
        SendOperation() : timestamp(std::chrono::steady_clock::now()) {}
        
        SendOperation(std::vector<uint8_t> d, SendCallback cb)
            : data(std::move(d)), callback(std::move(cb)), 
              timestamp(std::chrono::steady_clock::now()) {}
    };

    void StartReceive();
    void HandleReceive(boost::system::error_code ec, size_t bytes_transferred);
    
    void ProcessSendQueue();
    void HandleSend(boost::system::error_code ec, size_t bytes_transferred);
    
    void DoConnect(const std::string& host, const std::string& port,
                   std::function<void(boost::system::error_code)> callback);

    void HandleResolve(boost::system::error_code ec, resolver_type::results_type endpoints,
                      std::function<void(boost::system::error_code)> callback);

    void HandleConnect(boost::system::error_code ec, 
                      std::function<void(boost::system::error_code)> callback);

    void SetSocketOptions();
    void CloseSocket();

    // TCP socket and networking
    socket_type socket_;
    std::unique_ptr<resolver_type> resolver_;
    
    // Receive buffer
    static const size_t RECEIVE_BUFFER_SIZE = 8192;
    std::array<uint8_t, RECEIVE_BUFFER_SIZE> receive_buffer_;
    std::vector<uint8_t> message_buffer_; // For assembling partial messages
    
    // Send queue and synchronization
    std::queue<SendOperation> send_queue_;
    std::mutex send_mutex_;
    std::atomic<bool> sending_{false};
    
    // Connection state
    [[maybe_unused]] bool socket_owned_ = true; // Whether we own the socket (client) or it was passed to us (server)
    endpoint_type remote_endpoint_;
    endpoint_type local_endpoint_;
    
    // TCP options
    bool no_delay_ = true;
    bool keep_alive_ = true;
    size_t receive_buffer_size_ = 65536;
    size_t send_buffer_size_ = 65536;
};


} // namespace network
} // namespace common