#include "common/network/zeus_network.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <thread>
#include <random>
#include <unordered_map>
#include <mutex>

namespace common {
namespace network {

// NetworkModule static members
std::atomic<bool> NetworkModule::initialized_{false};
std::chrono::steady_clock::time_point NetworkModule::init_time_;
NetworkModule::ModuleStats NetworkModule::stats_;

bool NetworkModule::Initialize(const std::string& config_file, bool enable_logging) {
    if (initialized_.exchange(true)) {
        return true; // Already initialized
    }
    
    init_time_ = std::chrono::steady_clock::now();
    stats_.initialized_at = init_time_;
    
    // Initialize network logging
    if (enable_logging) {
        if (!NetworkLogger::Instance().Initialize("network", true)) {
            NETWORK_LOG_WARN("Failed to initialize network logger, continuing without logging");
        } else {
            NETWORK_LOG_INFO("Zeus Network Module v{} initialized", NetworkVersion::VERSION_STRING);
        }
    }
    
    // Initialize event manager
    NetworkEventManager::Instance().SetEventProcessingEnabled(true);
    
    NETWORK_LOG_INFO("Network module initialization completed");
    return true;
}

void NetworkModule::Shutdown() {
    if (!initialized_.exchange(false)) {
        return; // Already shutdown
    }
    
    NETWORK_LOG_INFO("Shutting down Zeus Network Module");
    
    // Clear all event hooks
    NetworkEventManager::Instance().ClearAllHooks();
    NetworkEventManager::Instance().SetEventProcessingEnabled(false);
    
    // Shutdown logging
    NetworkLogger::Instance().Shutdown();
    
    NETWORK_LOG_INFO("Network module shutdown completed");
}

bool NetworkModule::IsInitialized() {
    return initialized_.load();
}

NetworkModule::ModuleStats NetworkModule::GetStats() {
    auto current_stats = stats_;
    
    // Update hook statistics
    auto hook_stats = NetworkEventManager::Instance().GetHookStatistics();
    current_stats.total_registered_hooks = 0;
    for (const auto& [event_type, count] : hook_stats) {
        current_stats.total_registered_hooks += count;
    }
    
    return current_stats;
}

// NetworkUtils Implementation
namespace NetworkUtils {

std::string GenerateConnectionId(const std::string& prefix) {
    static std::atomic<uint64_t> counter{1};
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    
    std::ostringstream oss;
    oss << prefix << "_" << counter.fetch_add(1) << "_" 
        << std::hex << gen() << "_" 
        << std::hex << std::hash<std::thread::id>{}(std::this_thread::get_id());
    
    return oss.str();
}

std::pair<std::string, std::string> ParseEndpoint(const std::string& endpoint) {
    size_t colon_pos = endpoint.find_last_of(':');
    if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos == endpoint.length() - 1) {
        return {"", ""};
    }
    
    std::string host = endpoint.substr(0, colon_pos);
    std::string port = endpoint.substr(colon_pos + 1);
    
    // Basic validation
    if (host.empty() || port.empty()) {
        return {"", ""};
    }
    
    // Check port is numeric
    try {
        int port_num = std::stoi(port);
        if (port_num <= 0 || port_num > 65535) {
            return {"", ""};
        }
    } catch (...) {
        return {"", ""};
    }
    
    return {host, port};
}

bool IsValidEndpoint(const std::string& endpoint) {
    auto [host, port] = ParseEndpoint(endpoint);
    return !host.empty() && !port.empty();
}

std::vector<std::string> GetLocalIpAddresses() {
    std::vector<std::string> addresses;
    
    try {
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::resolver resolver(ioc);
        
        // Get local hostname
        boost::system::error_code ec;
        std::string hostname = boost::asio::ip::host_name(ec);
        
        if (!ec) {
            auto results = resolver.resolve(hostname, "", ec);
            if (!ec) {
                for (const auto& endpoint : results) {
                    addresses.push_back(endpoint.endpoint().address().to_string());
                }
            }
        }
        
        // Always include localhost
        if (std::find(addresses.begin(), addresses.end(), "127.0.0.1") == addresses.end()) {
            addresses.push_back("127.0.0.1");
        }
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to get local IP addresses: {}", e.what());
        addresses.push_back("127.0.0.1"); // Fallback
    }
    
    return addresses;
}

bool IsPortAvailable(uint16_t port, const std::string& protocol, const std::string& bind_address) {
    try {
        boost::asio::io_context ioc;
        
        if (protocol == "tcp") {
            boost::asio::ip::tcp::acceptor acceptor(ioc);
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(bind_address), port);
            
            boost::system::error_code ec;
            acceptor.open(endpoint.protocol(), ec);
            if (ec) return false;
            
            acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
            acceptor.bind(endpoint, ec);
            return !ec;
            
        } else if (protocol == "udp") {
            boost::asio::ip::udp::socket socket(ioc);
            boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::make_address(bind_address), port);
            
            boost::system::error_code ec;
            socket.open(endpoint.protocol(), ec);
            if (ec) return false;
            
            socket.set_option(boost::asio::ip::udp::socket::reuse_address(true), ec);
            socket.bind(endpoint, ec);
            return !ec;
        }
        
    } catch (const std::exception& e) {
        NETWORK_LOG_DEBUG("Port {} ({}) not available: {}", port, protocol, e.what());
    }
    
    return false;
}

uint16_t FindAvailablePort(uint16_t start_port, uint16_t end_port, 
                          const std::string& protocol, const std::string& bind_address) {
    for (uint16_t port = start_port; port <= end_port; ++port) {
        if (IsPortAvailable(port, protocol, bind_address)) {
            return port;
        }
    }
    return 0;
}

std::string BytesToString(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    const size_t num_units = sizeof(units) / sizeof(units[0]);
    
    double size = static_cast<double>(bytes);
    size_t unit_idx = 0;
    
    while (size >= 1024.0 && unit_idx < num_units - 1) {
        size /= 1024.0;
        unit_idx++;
    }
    
    std::ostringstream oss;
    if (unit_idx == 0) {
        oss << static_cast<uint64_t>(size) << " " << units[unit_idx];
    } else {
        oss << std::fixed << std::setprecision(1) << size << " " << units[unit_idx];
    }
    
    return oss.str();
}

std::string DurationToString(uint64_t duration_ms) {
    if (duration_ms < 1000) {
        return std::to_string(duration_ms) + "ms";
    }
    
    uint64_t seconds = duration_ms / 1000;
    uint64_t minutes = seconds / 60;
    uint64_t hours = minutes / 60;
    uint64_t days = hours / 24;
    
    std::ostringstream oss;
    
    if (days > 0) {
        oss << days << "d ";
        hours %= 24;
    }
    if (hours > 0) {
        oss << hours << "h ";
        minutes %= 60;
    }
    if (minutes > 0) {
        oss << minutes << "m ";
        seconds %= 60;
    }
    if (seconds > 0 || oss.str().empty()) {
        oss << seconds << "s";
    }
    
    std::string result = oss.str();
    if (result.back() == ' ') {
        result.pop_back();
    }
    
    return result;
}

} // namespace NetworkUtils

// CommonHooks Implementation
namespace CommonHooks {

void ConsoleLogger(const NetworkEvent& event) {
    std::string event_type;
    switch (event.type) {
        case NetworkEventType::CONNECTION_ESTABLISHED:
            event_type = "CONNECTED";
            break;
        case NetworkEventType::CONNECTION_FAILED:
            event_type = "CONNECT_FAILED";
            break;
        case NetworkEventType::CONNECTION_CLOSED:
            event_type = "DISCONNECTED";
            break;
        case NetworkEventType::CONNECTION_ERROR:
            event_type = "ERROR";
            break;
        case NetworkEventType::DATA_RECEIVED:
            event_type = "DATA_RX";
            break;
        case NetworkEventType::DATA_SENT:
            event_type = "DATA_TX";
            break;
        case NetworkEventType::DATA_SEND_ERROR:
            event_type = "SEND_ERROR";
            break;
        default:
            event_type = "OTHER";
            break;
    }
    
    std::cout << "[NETWORK] " << event_type << " - " 
              << event.protocol << " " << event.connection_id 
              << " (" << event.endpoint << ")";
    
    if (event.bytes_transferred > 0) {
        std::cout << " - " << NetworkUtils::BytesToString(event.bytes_transferred);
    }
    
    if (!event.error_message.empty()) {
        std::cout << " - Error: " << event.error_message;
    }
    
    std::cout << std::endl;
}

void ConnectionStatsTracker(const NetworkEvent& event) {
    static std::mutex stats_mutex;
    struct ProtocolStats {
        size_t connections_established = 0;
        size_t connections_failed = 0;
        size_t bytes_sent = 0;
        size_t bytes_received = 0;
        std::chrono::steady_clock::time_point last_activity;
    };
    static std::unordered_map<std::string, ProtocolStats> protocol_stats;
    
    std::lock_guard<std::mutex> lock(stats_mutex);
    auto& stats = protocol_stats[event.protocol];
    stats.last_activity = event.timestamp;
    
    switch (event.type) {
        case NetworkEventType::CONNECTION_ESTABLISHED:
            stats.connections_established++;
            break;
        case NetworkEventType::CONNECTION_FAILED:
            stats.connections_failed++;
            break;
        case NetworkEventType::DATA_SENT:
            stats.bytes_sent += event.bytes_transferred;
            break;
        case NetworkEventType::DATA_RECEIVED:
            stats.bytes_received += event.bytes_transferred;
            break;
        default:
            break;
    }
}

void ConnectionHealthMonitor(const NetworkEvent& event) {
    static std::mutex health_mutex;
    struct HealthInfo {
        size_t error_count = 0;
        std::chrono::steady_clock::time_point last_error;
        size_t consecutive_errors = 0;
    };
    static std::unordered_map<std::string, HealthInfo> connection_health;
    
    std::lock_guard<std::mutex> lock(health_mutex);
    auto& health = connection_health[event.connection_id];
    
    if (event.type == NetworkEventType::CONNECTION_ERROR || 
        event.type == NetworkEventType::DATA_SEND_ERROR) {
        
        health.error_count++;
        health.consecutive_errors++;
        health.last_error = event.timestamp;
        
        if (health.consecutive_errors >= 3) {
            NETWORK_LOG_WARN("Connection {} has {} consecutive errors, may be unstable",
                           event.connection_id, health.consecutive_errors);
        }
        
    } else if (event.type == NetworkEventType::DATA_SENT || 
               event.type == NetworkEventType::DATA_RECEIVED) {
        
        health.consecutive_errors = 0; // Reset on successful activity
    }
}

NetworkEventHook CreateRateLimitHook(size_t max_connections, uint32_t time_window_ms) {
    struct RateLimitData {
        std::mutex mutex;
        std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> connection_times;
    };
    
    auto data = std::make_shared<RateLimitData>();
    
    return [data, max_connections, time_window_ms](const NetworkEvent& event) {
        if (event.type != NetworkEventType::CONNECTION_ESTABLISHED) {
            return;
        }
        
        // Extract IP from endpoint
        auto [host, port] = NetworkUtils::ParseEndpoint(event.endpoint);
        if (host.empty()) {
            return;
        }
        
        std::lock_guard<std::mutex> lock(data->mutex);
        auto& times = data->connection_times[host];
        auto now = std::chrono::steady_clock::now();
        auto cutoff = now - std::chrono::milliseconds(time_window_ms);
        
        // Remove old entries
        times.erase(std::remove_if(times.begin(), times.end(),
                                  [cutoff](const auto& time) { return time < cutoff; }),
                   times.end());
        
        times.push_back(now);
        
        if (times.size() > max_connections) {
            NETWORK_LOG_WARN("Rate limit exceeded for IP {}: {} connections in {}ms window",
                           host, times.size(), time_window_ms);
        }
    };
}

NetworkEventHook CreateTimeoutMonitorHook(uint32_t timeout_ms) {
    struct TimeoutData {
        std::mutex mutex;
        std::unordered_map<std::string, std::chrono::steady_clock::time_point> last_activity;
    };
    
    auto data = std::make_shared<TimeoutData>();
    
    return [data, timeout_ms](const NetworkEvent& event) {
        std::lock_guard<std::mutex> lock(data->mutex);
        
        if (event.type == NetworkEventType::CONNECTION_ESTABLISHED ||
            event.type == NetworkEventType::DATA_SENT ||
            event.type == NetworkEventType::DATA_RECEIVED) {
            
            data->last_activity[event.connection_id] = event.timestamp;
            
        } else if (event.type == NetworkEventType::CONNECTION_CLOSED) {
            
            data->last_activity.erase(event.connection_id);
        }
        
        // Check for timeouts (simplified - in practice you'd use a timer)
        auto now = std::chrono::steady_clock::now();
        auto timeout_threshold = now - std::chrono::milliseconds(timeout_ms);
        
        for (auto it = data->last_activity.begin(); it != data->last_activity.end(); ) {
            if (it->second < timeout_threshold) {
                NETWORK_LOG_WARN("Connection {} appears to have timed out (no activity for {}ms)",
                               it->first, timeout_ms);
                it = data->last_activity.erase(it);
            } else {
                ++it;
            }
        }
    };
}

NetworkEventHook CreateFileLoggerHook(const std::string& filename) {
    auto file = std::make_shared<std::ofstream>(filename, std::ios::app);
    
    return [file, filename](const NetworkEvent& event) {
        if (!file->is_open()) {
            NETWORK_LOG_ERROR("Failed to write to log file: {}", filename);
            return;
        }
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        (*file) << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "] "
                << event.protocol << " " << event.connection_id << " "
                << static_cast<int>(event.type) << " " << event.endpoint;
        
        if (event.bytes_transferred > 0) {
            (*file) << " bytes=" << event.bytes_transferred;
        }
        
        if (!event.error_message.empty()) {
            (*file) << " error=\"" << event.error_message << "\"";
        }
        
        (*file) << std::endl;
        file->flush();
    };
}

} // namespace CommonHooks

// NetworkFactory Implementation
namespace NetworkFactory {

std::shared_ptr<TcpConnector> CreateTcpClient(
    boost::asio::any_io_executor executor,
    const std::string& connection_id,
    bool enable_keepalive,
    bool enable_nodelay) {
    
    std::string id = connection_id.empty() ? NetworkUtils::GenerateConnectionId("tcp") : connection_id;
    auto connection = std::make_shared<TcpConnector>(executor, id);
    
    connection->SetKeepAlive(enable_keepalive);
    connection->SetNoDelay(enable_nodelay);
    
    return connection;
}

std::shared_ptr<TcpAcceptor> CreateTcpServer(
    boost::asio::any_io_executor executor,
    uint16_t port,
    const std::string& bind_address,
    size_t max_connections) {
    
    auto server = std::make_shared<TcpAcceptor>(executor, port, bind_address);
    server->SetMaxConnections(max_connections);
    
    return server;
}

std::shared_ptr<KcpConnector> CreateKcpClient(
    boost::asio::any_io_executor executor,
    const std::string& connection_id,
    const KcpConnector::KcpConfig& config) {
    
    std::string id = connection_id.empty() ? NetworkUtils::GenerateConnectionId("kcp") : connection_id;
    return std::make_shared<KcpConnector>(executor, id, config);
}

std::shared_ptr<KcpAcceptor> CreateKcpServer(
    boost::asio::any_io_executor executor,
    uint16_t port,
    const std::string& bind_address,
    const KcpConnector::KcpConfig& config,
    size_t max_connections) {
    
    auto server = std::make_shared<KcpAcceptor>(executor, port, bind_address, config);
    server->SetMaxConnections(max_connections);
    
    return server;
}

} // namespace NetworkFactory

} // namespace network
} // namespace common