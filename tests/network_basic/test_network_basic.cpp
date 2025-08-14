#include "common/network/zeus_network.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

using namespace common::network;

// Global I/O context
boost::asio::io_context g_ioc;

void TestNetworkLogging() {
    std::cout << "\n=== Testing Network Logging ===" << std::endl;
    
    // Test network logging macros
    NETWORK_LOG_INFO("Network logging test started");
    NETWORK_LOG_DEBUG("This is a debug message with parameter: {}", 42);
    NETWORK_LOG_WARN("This is a warning message");
    NETWORK_LOG_ERROR("This is an error message with error code: {}", -1);
    
    // Test structured logging
    NetworkLogger::Instance().LogConnection("test_conn_1", "127.0.0.1:8080", "TCP");
    NetworkLogger::Instance().LogDataTransfer("test_conn_1", "send", 1024, "test_data");
    NetworkLogger::Instance().LogPerformance("test_conn_1", "latency", 15.5, "ms");
    NetworkLogger::Instance().LogError("test_conn_1", 10001, "Connection timeout");
    NetworkLogger::Instance().LogDisconnection("test_conn_1", "Client requested disconnect");
    
    std::cout << "Network logging test completed" << std::endl;
}

void TestNetworkEvents() {
    std::cout << "\n=== Testing Network Events and Hooks ===" << std::endl;
    
    // Register some test hooks
    auto hook_id1 = REGISTER_NETWORK_HOOK(
        {NetworkEventType::CONNECTION_ESTABLISHED, NetworkEventType::CONNECTION_CLOSED},
        "connection_tracker",
        [](const NetworkEvent& event) {
            std::cout << "Hook: Connection event - " << event.connection_id 
                     << " (" << static_cast<int>(event.type) << ")" << std::endl;
        }
    );
    
    auto hook_id2 = REGISTER_NETWORK_HOOK_WITH_FILTER(
        {NetworkEventType::DATA_SENT, NetworkEventType::DATA_RECEIVED},
        "data_tracker",
        [](const NetworkEvent& event) {
            std::cout << "Hook: Data transfer - " << event.bytes_transferred 
                     << " bytes on " << event.connection_id << std::endl;
        },
        EventFilters::ByMinDataSize(100) // Only track transfers >= 100 bytes
    );
    
    auto hook_id3 = REGISTER_GLOBAL_NETWORK_HOOK(
        "global_monitor",
        CommonHooks::ConsoleLogger
    );
    
    // Simulate some network events
    NetworkEvent connect_event(NetworkEventType::CONNECTION_ESTABLISHED);
    connect_event.connection_id = "test_conn_001";
    connect_event.endpoint = "192.168.1.100:9000";
    connect_event.protocol = "TCP";
    
    NetworkEventManager::Instance().FireEvent(connect_event);
    
    NetworkEvent data_event(NetworkEventType::DATA_RECEIVED);
    data_event.connection_id = "test_conn_001";
    data_event.endpoint = "192.168.1.100:9000";
    data_event.protocol = "TCP";
    data_event.bytes_transferred = 256;
    data_event.data = {'H', 'e', 'l', 'l', 'o'};
    
    NetworkEventManager::Instance().FireEvent(data_event);
    
    NetworkEvent small_data_event(NetworkEventType::DATA_SENT);
    small_data_event.connection_id = "test_conn_001";
    small_data_event.endpoint = "192.168.1.100:9000";
    small_data_event.protocol = "TCP";
    small_data_event.bytes_transferred = 50; // Should be filtered out
    
    NetworkEventManager::Instance().FireEvent(small_data_event);
    
    NetworkEvent disconnect_event(NetworkEventType::CONNECTION_CLOSED);
    disconnect_event.connection_id = "test_conn_001";
    disconnect_event.endpoint = "192.168.1.100:9000";
    disconnect_event.protocol = "TCP";
    
    NetworkEventManager::Instance().FireEvent(disconnect_event);
    
    // Test hook statistics
    auto stats = NetworkEventManager::Instance().GetHookStatistics();
    std::cout << "Hook statistics:" << std::endl;
    for (const auto& [event_type, count] : stats) {
        std::cout << "  Event type " << static_cast<int>(event_type) 
                 << ": " << count << " hooks" << std::endl;
    }
    
    // Cleanup hooks
    NetworkEventManager::Instance().UnregisterHook(hook_id1);
    NetworkEventManager::Instance().UnregisterHook(hook_id2);
    NetworkEventManager::Instance().UnregisterHook(hook_id3);
    
    std::cout << "Network events test completed" << std::endl;
}

void TestTcpBasicFunctionality() {
    std::cout << "\n=== Testing TCP Basic Functionality ===" << std::endl;
    
    // Test connection creation
    auto tcp_client = NetworkFactory::CreateTcpClient(g_ioc.get_executor(), "tcp_test_client");
    std::cout << "TCP client created: " << tcp_client->GetConnectionId() << std::endl;
    std::cout << "Protocol: " << tcp_client->GetProtocol() << std::endl;
    std::cout << "Initial state: " << static_cast<int>(tcp_client->GetState()) << std::endl;
    
    // Test configuration
    tcp_client->SetTimeout(10000); // 10 seconds
    tcp_client->SetHeartbeat(true, 30000); // 30 second heartbeat
    
    std::cout << "Timeout set to: " << tcp_client->GetTimeout() << "ms" << std::endl;
    
    // Test handlers
    tcp_client->SetDataHandler([](const std::vector<uint8_t>& data) {
        std::cout << "Data received: " << data.size() << " bytes" << std::endl;
    });
    
    tcp_client->SetErrorHandler([](boost::system::error_code ec) {
        std::cout << "Error occurred: " << ec.message() << std::endl;
    });
    
    tcp_client->SetStateChangeHandler([](ConnectionState old_state, ConnectionState new_state) {
        std::cout << "State changed: " << static_cast<int>(old_state) 
                 << " -> " << static_cast<int>(new_state) << std::endl;
    });
    
    // Test connection attempt (will fail since no server is running)
    std::cout << "Attempting to connect to localhost:12345 (expected to fail)..." << std::endl;
    tcp_client->AsyncConnect("localhost:12345", [](boost::system::error_code ec) {
        if (ec) {
            std::cout << "Connection failed as expected: " << ec.message() << std::endl;
        } else {
            std::cout << "Unexpected connection success!" << std::endl;
        }
    });
    
    // Give the connection attempt some time
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Process any pending async operations
    g_ioc.poll();
    
    std::cout << "Final state: " << static_cast<int>(tcp_client->GetState()) << std::endl;
    
    // Test connection statistics
    const auto& stats = tcp_client->GetStats();
    std::cout << "Connection stats:" << std::endl;
    std::cout << "  Bytes sent: " << stats.bytes_sent.load() << std::endl;
    std::cout << "  Bytes received: " << stats.bytes_received.load() << std::endl;
    std::cout << "  Messages sent: " << stats.messages_sent.load() << std::endl;
    std::cout << "  Messages received: " << stats.messages_received.load() << std::endl;
    std::cout << "  Errors: " << stats.errors_count.load() << std::endl;
    
    std::cout << "TCP basic functionality test completed" << std::endl;
}

void TestKcpBasicFunctionality() {
    std::cout << "\n=== Testing KCP Basic Functionality ===" << std::endl;
    
    // Test KCP configuration
    KcpConnector::KcpConfig config;
    config.conv_id = 12345;
    config.nodelay = 1;
    config.interval = 10;
    config.mtu = 1400;
    config.timeout_ms = 15000;
    
    // Test connection creation
    auto kcp_client = NetworkFactory::CreateKcpClient(g_ioc.get_executor(), "kcp_test_client", config);
    std::cout << "KCP client created: " << kcp_client->GetConnectionId() << std::endl;
    std::cout << "Protocol: " << kcp_client->GetProtocol() << std::endl;
    std::cout << "Conv ID: " << kcp_client->GetConfig().conv_id << std::endl;
    std::cout << "Initial state: " << static_cast<int>(kcp_client->GetState()) << std::endl;
    
    // Test handlers
    kcp_client->SetDataHandler([](const std::vector<uint8_t>& data) {
        std::cout << "KCP data received: " << data.size() << " bytes" << std::endl;
    });
    
    kcp_client->SetErrorHandler([](boost::system::error_code ec) {
        std::cout << "KCP error occurred: " << ec.message() << std::endl;
    });
    
    kcp_client->SetStateChangeHandler([](ConnectionState old_state, ConnectionState new_state) {
        std::cout << "KCP state changed: " << static_cast<int>(old_state) 
                 << " -> " << static_cast<int>(new_state) << std::endl;
    });
    
    // Test connection attempt (will fail since no server is running)
    std::cout << "Attempting KCP connection to localhost:12346 (expected to fail)..." << std::endl;
    kcp_client->AsyncConnect("localhost:12346", [](boost::system::error_code ec) {
        if (ec) {
            std::cout << "KCP connection failed as expected: " << ec.message() << std::endl;
        } else {
            std::cout << "Unexpected KCP connection success!" << std::endl;
        }
    });
    
    // Give the connection attempt some time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Process any pending async operations
    g_ioc.poll();
    
    std::cout << "KCP final state: " << static_cast<int>(kcp_client->GetState()) << std::endl;
    
    // Test KCP statistics
    auto kcp_stats = kcp_client->GetKcpStats();
    std::cout << "KCP stats:" << std::endl;
    std::cout << "  Packets sent: " << kcp_stats.packets_sent << std::endl;
    std::cout << "  Packets received: " << kcp_stats.packets_received << std::endl;
    std::cout << "  Bytes sent: " << kcp_stats.bytes_sent << std::endl;
    std::cout << "  Bytes received: " << kcp_stats.bytes_received << std::endl;
    std::cout << "  Average RTT: " << kcp_stats.rtt_avg << "ms" << std::endl;
    
    std::cout << "KCP basic functionality test completed" << std::endl;
}

void TestNetworkUtilities() {
    std::cout << "\n=== Testing Network Utilities ===" << std::endl;
    
    // Test endpoint parsing
    auto [host1, port1] = NetworkUtils::ParseEndpoint("192.168.1.1:8080");
    std::cout << "Parsed '192.168.1.1:8080': host='" << host1 << "', port='" << port1 << "'" << std::endl;
    
    auto [host2, port2] = NetworkUtils::ParseEndpoint("invalid_endpoint");
    std::cout << "Parsed 'invalid_endpoint': host='" << host2 << "', port='" << port2 << "'" << std::endl;
    
    // Test endpoint validation
    std::cout << "Is '127.0.0.1:9000' valid? " << NetworkUtils::IsValidEndpoint("127.0.0.1:9000") << std::endl;
    std::cout << "Is 'invalid' valid? " << NetworkUtils::IsValidEndpoint("invalid") << std::endl;
    
    // Test connection ID generation
    for (int i = 0; i < 3; ++i) {
        std::cout << "Generated ID " << i+1 << ": " << NetworkUtils::GenerateConnectionId("test") << std::endl;
    }
    
    // Test byte formatting
    std::cout << "Bytes formatting:" << std::endl;
    std::cout << "  1024 bytes = " << NetworkUtils::BytesToString(1024) << std::endl;
    std::cout << "  1048576 bytes = " << NetworkUtils::BytesToString(1048576) << std::endl;
    std::cout << "  1073741824 bytes = " << NetworkUtils::BytesToString(1073741824ULL) << std::endl;
    
    // Test duration formatting
    std::cout << "Duration formatting:" << std::endl;
    std::cout << "  500ms = " << NetworkUtils::DurationToString(500) << std::endl;
    std::cout << "  5000ms = " << NetworkUtils::DurationToString(5000) << std::endl;
    std::cout << "  65000ms = " << NetworkUtils::DurationToString(65000) << std::endl;
    std::cout << "  3665000ms = " << NetworkUtils::DurationToString(3665000) << std::endl;
    
    // Test local IP addresses
    std::cout << "Local IP addresses:" << std::endl;
    auto local_ips = NetworkUtils::GetLocalIpAddresses();
    for (const auto& ip : local_ips) {
        std::cout << "  " << ip << std::endl;
    }
    
    // Test port availability (check some common ports)
    std::cout << "Port availability check:" << std::endl;
    std::cout << "  Port 80 (TCP): " << (NetworkUtils::IsPortAvailable(80, "tcp") ? "available" : "in use") << std::endl;
    std::cout << "  Port 8080 (TCP): " << (NetworkUtils::IsPortAvailable(8080, "tcp") ? "available" : "in use") << std::endl;
    std::cout << "  Port 53 (UDP): " << (NetworkUtils::IsPortAvailable(53, "udp") ? "available" : "in use") << std::endl;
    
    // Test finding available port
    uint16_t available_port = NetworkUtils::FindAvailablePort(9000, 9010, "tcp");
    if (available_port > 0) {
        std::cout << "Found available TCP port in range 9000-9010: " << available_port << std::endl;
    } else {
        std::cout << "No available TCP ports found in range 9000-9010" << std::endl;
    }
    
    std::cout << "Network utilities test completed" << std::endl;
}

void TestModuleManagement() {
    std::cout << "\n=== Testing Module Management ===" << std::endl;
    
    // Test module statistics
    auto stats = NetworkModule::GetStats();
    std::cout << "Module statistics:" << std::endl;
    std::cout << "  Active TCP connections: " << stats.active_tcp_connections << std::endl;
    std::cout << "  Active KCP connections: " << stats.active_kcp_connections << std::endl;
    std::cout << "  Total registered hooks: " << stats.total_registered_hooks << std::endl;
    std::cout << "  Total bytes sent: " << stats.total_bytes_sent << std::endl;
    std::cout << "  Total bytes received: " << stats.total_bytes_received << std::endl;
    
    // Test version info
    std::cout << "Network module version: " << NetworkModule::GetVersion() << std::endl;
    std::cout << "Is initialized: " << NetworkModule::IsInitialized() << std::endl;
    
    std::cout << "Module management test completed" << std::endl;
}

int main() {
    std::cout << "Zeus Network Module Basic Test" << std::endl;
    std::cout << "==============================" << std::endl;
    
    // Initialize the network module
    if (!ZEUS_NETWORK_INIT("network_log_config.json")) {
        std::cerr << "Failed to initialize network module" << std::endl;
        return 1;
    }
    
    std::cout << "Network module initialized successfully" << std::endl;
    
    try {
        // Run all tests
        TestNetworkLogging();
        TestNetworkEvents();
        TestTcpBasicFunctionality();
        TestKcpBasicFunctionality();
        TestNetworkUtilities();
        TestModuleManagement();
        
        std::cout << "\n=== All Tests Completed Successfully ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        ZEUS_NETWORK_SHUTDOWN();
        return 1;
    }
    
    // Process any remaining async operations
    std::cout << "\nProcessing remaining async operations..." << std::endl;
    g_ioc.run_for(std::chrono::milliseconds(500));
    
    // Cleanup
    std::cout << "Shutting down network module..." << std::endl;
    ZEUS_NETWORK_SHUTDOWN();
    
    std::cout << "Test program completed successfully" << std::endl;
    return 0;
}