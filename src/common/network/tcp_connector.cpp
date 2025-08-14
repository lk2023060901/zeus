#include "common/network/tcp_connector.h"
#include "common/network/network_logger.h"
#include <boost/asio/connect.hpp>
#include <sstream>
#include <iomanip>

namespace common {
namespace network {

// TcpConnector Implementation
TcpConnector::TcpConnector(boost::asio::any_io_executor executor, const std::string& connection_id)
    : Connection(executor, connection_id), socket_(executor), socket_owned_(true) {
    
    resolver_ = std::make_unique<resolver_type>(executor);
    SetSocketOptions();
    
    NETWORK_LOG_DEBUG("TCP client connection created: {}", connection_id);
}

TcpConnector::TcpConnector(socket_type socket, const std::string& connection_id)
    : Connection(socket.get_executor(), connection_id), socket_(std::move(socket)), socket_owned_(false) {
    
    // Store endpoints
    boost::system::error_code ec;
    remote_endpoint_ = socket_.remote_endpoint(ec);
    if (!ec) {
        local_endpoint_ = socket_.local_endpoint(ec);
    }
    
    SetSocketOptions();
    UpdateState(ConnectionState::CONNECTED);
    StartReceive();
    
    NETWORK_LOG_INFO("TCP server connection created: {} from {}", connection_id, GetRemoteEndpoint());
}

TcpConnector::~TcpConnector() {
    CloseSocket();
    NETWORK_LOG_DEBUG("TCP connection destroyed: {}", connection_id_);
}

std::string TcpConnector::GetRemoteEndpoint() const {
    if (remote_endpoint_ != endpoint_type{}) {
        std::ostringstream oss;
        oss << remote_endpoint_.address().to_string() << ":" << remote_endpoint_.port();
        return oss.str();
    }
    return "unknown";
}

std::string TcpConnector::GetLocalEndpoint() const {
    if (local_endpoint_ != endpoint_type{}) {
        std::ostringstream oss;
        oss << local_endpoint_.address().to_string() << ":" << local_endpoint_.port();
        return oss.str();
    }
    return "unknown";
}

void TcpConnector::AsyncConnect(const std::string& endpoint, 
                                std::function<void(boost::system::error_code)> callback) {
    if (GetState() != ConnectionState::DISCONNECTED) {
        NETWORK_LOG_WARN("Attempting to connect already connected TCP connection: {}", connection_id_);
        if (callback) {
            boost::asio::post(executor_, [callback]() {
                callback(boost::asio::error::already_connected);
            });
        }
        return;
    }

    UpdateState(ConnectionState::CONNECTING);
    
    // Parse endpoint (host:port)
    size_t colon_pos = endpoint.find(':');
    if (colon_pos == std::string::npos) {
        NETWORK_LOG_ERROR("Invalid endpoint format for TCP connection {}: {}", connection_id_, endpoint);
        UpdateState(ConnectionState::ERROR);
        if (callback) {
            boost::asio::post(executor_, [callback]() {
                callback(boost::asio::error::invalid_argument);
            });
        }
        return;
    }
    
    std::string host = endpoint.substr(0, colon_pos);
    std::string port = endpoint.substr(colon_pos + 1);
    
    NETWORK_LOG_INFO("TCP connection {} attempting to connect to {}:{}", connection_id_, host, port);
    
    DoConnect(host, port, callback);
}

void TcpConnector::AsyncSend(const std::vector<uint8_t>& data, SendCallback callback) {
    if (!IsConnected()) {
        NETWORK_LOG_WARN("Attempting to send on disconnected TCP connection: {}", connection_id_);
        if (callback) {
            boost::asio::post(executor_, [callback]() {
                callback(boost::asio::error::not_connected, 0);
            });
        }
        return;
    }
    
    if (data.empty()) {
        NETWORK_LOG_WARN("Attempting to send empty data on TCP connection: {}", connection_id_);
        if (callback) {
            boost::asio::post(executor_, [callback]() {
                callback(boost::system::error_code{}, 0);
            });
        }
        return;
    }
    
    // Add to send queue
    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        send_queue_.emplace(data, callback);
    }
    
    ProcessSendQueue();
}

void TcpConnector::Close() {
    if (GetState() == ConnectionState::DISCONNECTED || GetState() == ConnectionState::DISCONNECTING) {
        return;
    }
    
    UpdateState(ConnectionState::DISCONNECTING);
    
    NETWORK_LOG_INFO("Gracefully closing TCP connection: {}", connection_id_);
    
    // Shutdown the socket gracefully
    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    if (ec) {
        NETWORK_LOG_DEBUG("TCP shutdown error on connection {}: {}", connection_id_, ec.message());
    }
    
    // Close after a short delay to allow pending operations to complete
    auto self = shared_from_this();
    auto timer = std::make_shared<boost::asio::steady_timer>(executor_);
    timer->expires_after(std::chrono::milliseconds(100));
    timer->async_wait([self, timer](boost::system::error_code) {
        self->Close();
    });
}

void TcpConnector::ForceClose() {
    NETWORK_LOG_INFO("Force closing TCP connection: {}", connection_id_);
    CloseSocket();
}

void TcpConnector::SetNoDelay(bool enable) {
    no_delay_ = enable;
    if (socket_.is_open()) {
        boost::system::error_code ec;
        socket_.set_option(boost::asio::ip::tcp::no_delay(enable), ec);
        if (ec) {
            NETWORK_LOG_WARN("Failed to set TCP_NODELAY on connection {}: {}", connection_id_, ec.message());
        }
    }
}

void TcpConnector::SetKeepAlive(bool enable, uint32_t idle_time, uint32_t interval, uint32_t probes) {
    keep_alive_ = enable;
    if (socket_.is_open()) {
        boost::system::error_code ec;
        socket_.set_option(boost::asio::socket_base::keep_alive(enable), ec);
        if (ec) {
            NETWORK_LOG_WARN("Failed to set SO_KEEPALIVE on connection {}: {}", connection_id_, ec.message());
        }
        
        // Platform-specific keepalive settings would go here
        // This requires platform-specific socket options
    }
}

void TcpConnector::SetReceiveBufferSize(size_t size) {
    receive_buffer_size_ = size;
    if (socket_.is_open()) {
        boost::system::error_code ec;
        socket_.set_option(boost::asio::socket_base::receive_buffer_size(static_cast<int>(size)), ec);
        if (ec) {
            NETWORK_LOG_WARN("Failed to set receive buffer size on connection {}: {}", connection_id_, ec.message());
        }
    }
}

void TcpConnector::SetSendBufferSize(size_t size) {
    send_buffer_size_ = size;
    if (socket_.is_open()) {
        boost::system::error_code ec;
        socket_.set_option(boost::asio::socket_base::send_buffer_size(static_cast<int>(size)), ec);
        if (ec) {
            NETWORK_LOG_WARN("Failed to set send buffer size on connection {}: {}", connection_id_, ec.message());
        }
    }
}

void TcpConnector::SetReuseAddress(bool enable) {
    if (socket_.is_open()) {
        boost::system::error_code ec;
        socket_.set_option(boost::asio::socket_base::reuse_address(enable), ec);
        if (ec) {
            NETWORK_LOG_WARN("Failed to set SO_REUSEADDR on connection {}: {}", connection_id_, ec.message());
        }
    }
}

void TcpConnector::SendHeartbeat() {
    // Send a simple heartbeat message (can be customized)
    std::vector<uint8_t> heartbeat_data = {'H', 'B'}; // "HB" for heartbeat
    AsyncSend(heartbeat_data, [this](boost::system::error_code ec, size_t) {
        if (ec) {
            NETWORK_LOG_WARN("Heartbeat send failed on connection {}: {}", connection_id_, ec.message());
        } else {
            NETWORK_LOG_TRACE("Heartbeat sent on connection {}", connection_id_);
        }
    });
}

void TcpConnector::StartReceive() {
    if (!IsConnected()) {
        return;
    }
    
    auto self = std::static_pointer_cast<TcpConnector>(shared_from_this());
    socket_.async_receive(
        boost::asio::buffer(receive_buffer_),
        [self](boost::system::error_code ec, size_t bytes_transferred) {
            self->HandleReceive(ec, bytes_transferred);
        });
}

void TcpConnector::HandleReceive(boost::system::error_code ec, size_t bytes_transferred) {
    if (ec) {
        if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
            NETWORK_LOG_INFO("TCP connection {} closed by peer", connection_id_);
            UpdateState(ConnectionState::DISCONNECTED);
        } else {
            NETWORK_LOG_ERROR("TCP receive error on connection {}: {}", connection_id_, ec.message());
            HandleError(ec);
        }
        return;
    }
    
    if (bytes_transferred > 0) {
        // Copy received data to message buffer
        std::vector<uint8_t> received_data(
            receive_buffer_.begin(), 
            receive_buffer_.begin() + bytes_transferred
        );
        
        HandleDataReceived(received_data);
        
        NetworkLogger::Instance().LogDataTransfer(connection_id_, "receive", bytes_transferred, "TCP");
    }
    
    // Continue receiving
    StartReceive();
}

void TcpConnector::ProcessSendQueue() {
    // Only one send operation at a time
    if (sending_.exchange(true)) {
        return;
    }
    
    SendOperation* operation = nullptr;
    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        if (!send_queue_.empty()) {
            // Get pointer to front operation (we'll process it outside the lock)
            operation = &send_queue_.front();
        }
    }
    
    if (!operation) {
        sending_ = false;
        return;
    }
    
    auto self = std::static_pointer_cast<TcpConnector>(shared_from_this());
    boost::asio::async_write(
        socket_,
        boost::asio::buffer(operation->data),
        [self](boost::system::error_code ec, size_t bytes_transferred) {
            self->HandleSend(ec, bytes_transferred);
        });
}

void TcpConnector::HandleSend(boost::system::error_code ec, size_t bytes_transferred) {
    SendOperation completed_operation;
    
    {
        std::lock_guard<std::mutex> lock(send_mutex_);
        if (!send_queue_.empty()) {
            completed_operation = std::move(send_queue_.front());
            send_queue_.pop();
        }
    }
    
    sending_ = false;
    
    if (ec) {
        NETWORK_LOG_ERROR("TCP send error on connection {}: {}", connection_id_, ec.message());
        HandleError(ec);
        
        if (completed_operation.callback) {
            completed_operation.callback(ec, 0);
        }
        return;
    }
    
    // Update statistics
    stats_.bytes_sent.fetch_add(bytes_transferred);
    stats_.messages_sent.fetch_add(1);
    
    NetworkLogger::Instance().LogDataTransfer(connection_id_, "send", bytes_transferred, "TCP");
    
    // Call completion callback
    if (completed_operation.callback) {
        completed_operation.callback(ec, bytes_transferred);
    }
    
    // Fire send event
    NetworkEvent event(NetworkEventType::DATA_SENT);
    event.connection_id = connection_id_;
    event.endpoint = GetRemoteEndpoint();
    event.protocol = GetProtocol();
    event.connection = shared_from_this();
    event.bytes_transferred = bytes_transferred;
    FireEvent(event);
    
    // Process next item in queue
    ProcessSendQueue();
}

void TcpConnector::DoConnect(const std::string& host, const std::string& port,
                             std::function<void(boost::system::error_code)> callback) {
    
    auto self = std::static_pointer_cast<TcpConnector>(shared_from_this());
    resolver_->async_resolve(
        host, port,
        [self, callback](boost::system::error_code ec, resolver_type::results_type endpoints) {
            self->HandleResolve(ec, endpoints, callback);
        });
}

void TcpConnector::HandleResolve(boost::system::error_code ec, resolver_type::results_type endpoints,
                                 std::function<void(boost::system::error_code)> callback) {
    if (ec) {
        NETWORK_LOG_ERROR("DNS resolution failed for TCP connection {}: {}", connection_id_, ec.message());
        UpdateState(ConnectionState::ERROR);
        if (callback) callback(ec);
        return;
    }
    
    auto self = std::static_pointer_cast<TcpConnector>(shared_from_this());
    boost::asio::async_connect(
        socket_, endpoints,
        [self, callback](boost::system::error_code ec, const endpoint_type& endpoint) {
            if (!ec) {
                self->remote_endpoint_ = endpoint;
                boost::system::error_code local_ec;
                self->local_endpoint_ = self->socket_.local_endpoint(local_ec);
            }
            self->HandleConnect(ec, callback);
        });
}

void TcpConnector::HandleConnect(boost::system::error_code ec, 
                                 std::function<void(boost::system::error_code)> callback) {
    if (ec) {
        NETWORK_LOG_ERROR("TCP connect failed for connection {}: {}", connection_id_, ec.message());
        UpdateState(ConnectionState::ERROR);
    } else {
        NETWORK_LOG_INFO("TCP connection established: {} to {}", connection_id_, GetRemoteEndpoint());
        UpdateState(ConnectionState::CONNECTED);
        StartReceive();
    }
    
    if (callback) {
        callback(ec);
    }
}

void TcpConnector::SetSocketOptions() {
    if (!socket_.is_open()) {
        return;
    }
    
    boost::system::error_code ec;
    
    // Set common socket options
    socket_.set_option(boost::asio::ip::tcp::no_delay(no_delay_), ec);
    socket_.set_option(boost::asio::socket_base::keep_alive(keep_alive_), ec);
    socket_.set_option(boost::asio::socket_base::receive_buffer_size(static_cast<int>(receive_buffer_size_)), ec);
    socket_.set_option(boost::asio::socket_base::send_buffer_size(static_cast<int>(send_buffer_size_)), ec);
    
    // Suppress error logging for common option setting failures
    // as not all platforms support all options
}

void TcpConnector::CloseSocket() {
    if (socket_.is_open()) {
        boost::system::error_code ec;
        socket_.close(ec);
        UpdateState(ConnectionState::DISCONNECTED);
        
        NETWORK_LOG_DEBUG("TCP socket closed for connection: {}", connection_id_);
    }
}


} // namespace network
} // namespace common