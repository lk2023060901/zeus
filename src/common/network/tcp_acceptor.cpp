#include "common/network/tcp_acceptor.h"
#include "common/network/network_logger.h"
#include <sstream>
#include <iomanip>
#include <thread>

namespace common {
namespace network {

TcpAcceptor::TcpAcceptor(boost::asio::any_io_executor executor, uint16_t port, const std::string& bind_address)
    : executor_(executor), acceptor_(executor), port_(port), bind_address_(bind_address), accept_socket_(executor) {
    
    NETWORK_LOG_INFO("TCP server created for {}:{}", bind_address_, port_);
}

TcpAcceptor::~TcpAcceptor() {
    Stop();
    NETWORK_LOG_INFO("TCP server destroyed");
}

bool TcpAcceptor::Start(connection_handler_type connection_handler) {
    if (running_) {
        NETWORK_LOG_WARN("TCP server already running");
        return false;
    }
    
    connection_handler_ = std::move(connection_handler);
    
    try {
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(bind_address_), port_);
        
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen();
        
        running_ = true;
        
        NETWORK_LOG_INFO("TCP server listening on {}", GetListeningEndpoint());
        
        DoAccept();
        return true;
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to start TCP server: {}", e.what());
        return false;
    }
}

void TcpAcceptor::Stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    boost::system::error_code ec;
    acceptor_.close(ec);
    
    NETWORK_LOG_INFO("TCP server stopped, had {} active connections", active_connections_.load());
}

std::string TcpAcceptor::GetListeningEndpoint() const {
    if (acceptor_.is_open()) {
        auto endpoint = acceptor_.local_endpoint();
        std::ostringstream oss;
        oss << endpoint.address().to_string() << ":" << endpoint.port();
        return oss.str();
    }
    return "not_listening";
}

void TcpAcceptor::SetConnectionFactory(connection_factory_type factory) {
    connection_factory_ = std::move(factory);
}

size_t TcpAcceptor::GetConnectionCount() const {
    return active_connections_.load();
}

void TcpAcceptor::SetMaxConnections(size_t max_connections) {
    max_connections_ = max_connections;
    NETWORK_LOG_INFO("TCP server max connections set to {}", max_connections);
}

void TcpAcceptor::DoAccept() {
    if (!running_) {
        return;
    }
    
    accept_socket_ = TcpConnector::socket_type(executor_);
    
    acceptor_.async_accept(accept_socket_,
        [this](boost::system::error_code ec) {
            HandleAccept(ec);
        });
}

void TcpAcceptor::HandleAccept(boost::system::error_code ec) {
    if (ec) {
        if (running_) {
            NETWORK_LOG_ERROR("TCP accept error: {}", ec.message());
            // Continue accepting
            DoAccept();
        }
        return;
    }
    
    // Check connection limits
    if (max_connections_ > 0 && active_connections_.load() >= max_connections_) {
        NETWORK_LOG_WARN("TCP server rejecting connection - maximum connections reached ({})", max_connections_);
        boost::system::error_code close_ec;
        accept_socket_.close(close_ec);
        DoAccept();
        return;
    }
    
    // Create connection
    std::string connection_id = GenerateConnectionId();
    std::shared_ptr<TcpConnector> connection;
    
    if (connection_factory_) {
        connection = connection_factory_(std::move(accept_socket_));
    } else {
        connection = std::make_shared<TcpConnector>(std::move(accept_socket_), connection_id);
    }
    
    if (connection) {
        active_connections_.fetch_add(1);
        
        // Set up connection cleanup when it's destroyed
        // Note: This is a simplified approach - a production implementation might
        // use weak_ptr callbacks or connection pools
        
        NETWORK_LOG_INFO("TCP server accepted connection: {} from {}", 
                        connection->GetConnectionId(), connection->GetRemoteEndpoint());
        
        if (connection_handler_) {
            connection_handler_(connection);
        }
    }
    
    // Continue accepting
    DoAccept();
}

std::string TcpAcceptor::GenerateConnectionId() {
    std::ostringstream oss;
    oss << "tcp_" << connection_counter_.fetch_add(1) << "_" << std::hex << 
           std::hash<std::thread::id>{}(std::this_thread::get_id());
    return oss.str();
}

} // namespace network
} // namespace common