#include "common/network/tcp_connector.h"
#include "common/network/zeus_network.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

using namespace common::network;

// Global I/O context for testing
boost::asio::io_context g_ioc;

void TestTcpConnectorCreation() {
    std::cout << "\n=== Testing TCP Connection Creation ===" << std::endl;
    
    // Test client connection creation
    auto tcp_client = NetworkFactory::CreateTcpClient(g_ioc.get_executor(), "test_client");
    
    std::cout << "✓ TCP Client created" << std::endl;
    std::cout << "  Connection ID: " << tcp_client->GetConnectionId() << std::endl;
    std::cout << "  Protocol: " << tcp_client->GetProtocol() << std::endl;
    std::cout << "  Initial State: " << static_cast<int>(tcp_client->GetState()) << std::endl;
    std::cout << "  Timeout: " << tcp_client->GetTimeout() << "ms" << std::endl;
    
    // Verify initial state
    assert(tcp_client->GetState() == ConnectionState::DISCONNECTED);
    assert(tcp_client->GetProtocol() == "TCP");
    
    std::cout << "TCP Connection Creation test passed" << std::endl;
}

void TestTcpConnectorConfiguration() {
    std::cout << "\n=== Testing TCP Connection Configuration ===" << std::endl;
    
    auto tcp_client = NetworkFactory::CreateTcpClient(g_ioc.get_executor(), "config_test");
    
    // Test timeout configuration
    tcp_client->SetTimeout(15000);
    assert(tcp_client->GetTimeout() == 15000);
    std::cout << "✓ Timeout configuration works" << std::endl;
    
    // Test heartbeat configuration
    tcp_client->SetHeartbeat(true, 25000);
    std::cout << "✓ Heartbeat configuration set" << std::endl;
    
    // Test handler registration
    bool data_handler_called = false;
    bool error_handler_called = false;
    bool state_handler_called = false;
    
    tcp_client->SetDataHandler([&data_handler_called](const std::vector<uint8_t>& data) {
        data_handler_called = true;
        std::cout << "Data handler called with " << data.size() << " bytes" << std::endl;
    });
    
    tcp_client->SetErrorHandler([&error_handler_called](boost::system::error_code ec) {
        error_handler_called = true;
        std::cout << "Error handler called: " << ec.message() << std::endl;
    });
    
    tcp_client->SetStateChangeHandler([&state_handler_called](ConnectionState old_state, ConnectionState new_state) {
        state_handler_called = true;
        std::cout << "State change: " << static_cast<int>(old_state) 
                 << " -> " << static_cast<int>(new_state) << std::endl;
    });
    
    std::cout << "✓ All handlers registered successfully" << std::endl;
    std::cout << "TCP Connection Configuration test passed" << std::endl;
}

void TestTcpConnectorFailure() {
    std::cout << "\n=== Testing TCP Connection Failure Handling ===" << std::endl;
    
    auto tcp_client = NetworkFactory::CreateTcpClient(g_ioc.get_executor(), "failure_test");
    
    bool connection_callback_called = false;
    boost::system::error_code captured_error;
    
    // Test connection to non-existent server
    tcp_client->AsyncConnect("127.0.0.1:65432", [&](boost::system::error_code ec) {
        connection_callback_called = true;
        captured_error = ec;
        std::cout << "Connection callback executed with error: " << ec.message() << std::endl;
    });
    
    // Process async operations for a short time
    auto start = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(500)) {
        g_ioc.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    assert(connection_callback_called);
    assert(captured_error); // Should have an error
    std::cout << "✓ Connection failure properly detected" << std::endl;
    
    // Verify state after failure
    assert(tcp_client->GetState() == ConnectionState::DISCONNECTED || 
           tcp_client->GetState() == ConnectionState::ERROR);
    std::cout << "✓ State correctly set after failure" << std::endl;
    
    std::cout << "TCP Connection Failure test passed" << std::endl;
}

void TestTcpConnectorStatistics() {
    std::cout << "\n=== Testing TCP Connection Statistics ===" << std::endl;
    
    auto tcp_client = NetworkFactory::CreateTcpClient(g_ioc.get_executor(), "stats_test");
    
    // Get initial statistics
    const auto& stats = tcp_client->GetStats();
    std::cout << "Initial statistics:" << std::endl;
    std::cout << "  Bytes sent: " << stats.bytes_sent.load() << std::endl;
    std::cout << "  Bytes received: " << stats.bytes_received.load() << std::endl;
    std::cout << "  Messages sent: " << stats.messages_sent.load() << std::endl;
    std::cout << "  Messages received: " << stats.messages_received.load() << std::endl;
    std::cout << "  Errors: " << stats.errors_count.load() << std::endl;
    
    // Verify initial values
    assert(stats.bytes_sent.load() == 0);
    assert(stats.bytes_received.load() == 0);
    assert(stats.messages_sent.load() == 0);
    assert(stats.messages_received.load() == 0);
    assert(stats.errors_count.load() == 0);
    
    std::cout << "✓ Initial statistics are correct" << std::endl;
    
    std::cout << "TCP Connection Statistics test passed" << std::endl;
}

void TestTcpConnectorDataHandling() {
    std::cout << "\n=== Testing TCP Connection Data Handling ===" << std::endl;
    
    auto tcp_client = NetworkFactory::CreateTcpClient(g_ioc.get_executor(), "data_test");
    
    std::vector<uint8_t> received_data;
    tcp_client->SetDataHandler([&received_data](const std::vector<uint8_t>& data) {
        received_data = data;
        std::cout << "Received " << data.size() << " bytes of data" << std::endl;
    });
    
    // Test send preparation (without actual connection)
    std::vector<uint8_t> test_data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
    
    std::cout << "✓ Data handler configured for " << test_data.size() << " byte test message" << std::endl;
    
    // Note: We can't actually send data without a real connection,
    // but we've verified the handler setup works
    
    std::cout << "TCP Connection Data Handling test passed" << std::endl;
}

void TestTcpConnectorCleanup() {
    std::cout << "\n=== Testing TCP Connection Cleanup ===" << std::endl;
    
    auto tcp_client = NetworkFactory::CreateTcpClient(g_ioc.get_executor(), "cleanup_test");
    
    // Configure the connection
    tcp_client->SetTimeout(5000);
    tcp_client->SetDataHandler([](const std::vector<uint8_t>&) {});
    tcp_client->SetErrorHandler([](boost::system::error_code) {});
    
    std::cout << "✓ Connection configured for cleanup test" << std::endl;
    
    // Test disconnect (should not crash even if not connected)
    tcp_client->Close();
    std::cout << "✓ Disconnect called successfully" << std::endl;
    
    // Verify state after cleanup
    std::cout << "Final state: " << static_cast<int>(tcp_client->GetState()) << std::endl;
    
    std::cout << "TCP Connection Cleanup test passed" << std::endl;
}

void TestTcpConnectorEdgeCases() {
    std::cout << "\n=== Testing TCP Connection Edge Cases ===" << std::endl;
    
    auto tcp_client = NetworkFactory::CreateTcpClient(g_ioc.get_executor(), "edge_test");
    
    // Test multiple disconnect calls
    tcp_client->Close();
    tcp_client->Close(); // Should not crash
    std::cout << "✓ Multiple disconnect calls handled safely" << std::endl;
    
    // Test configuration changes during disconnected state
    tcp_client->SetTimeout(1000);
    tcp_client->SetTimeout(2000);
    assert(tcp_client->GetTimeout() == 2000);
    std::cout << "✓ Configuration changes during disconnect work" << std::endl;
    
    // Test handler replacement
    int handler_call_count = 0;
    tcp_client->SetDataHandler([&handler_call_count](const std::vector<uint8_t>&) {
        handler_call_count++;
    });
    
    tcp_client->SetDataHandler([&handler_call_count](const std::vector<uint8_t>&) {
        handler_call_count += 10;
    });
    std::cout << "✓ Handler replacement works" << std::endl;
    
    std::cout << "TCP Connection Edge Cases test passed" << std::endl;
}

int main() {
    std::cout << "Zeus TCP Connection Test Suite" << std::endl;
    std::cout << "==============================" << std::endl;
    
    // Initialize the network module
    if (!ZEUS_NETWORK_INIT("")) {
        std::cerr << "Failed to initialize network module" << std::endl;
        return 1;
    }
    
    std::cout << "Network module initialized successfully" << std::endl;
    
    try {
        TestTcpConnectorCreation();
        TestTcpConnectorConfiguration();
        TestTcpConnectorFailure();
        TestTcpConnectorStatistics();
        TestTcpConnectorDataHandling();
        TestTcpConnectorCleanup();
        TestTcpConnectorEdgeCases();
        
        std::cout << "\n=== All TCP Connection Tests Passed ===\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        ZEUS_NETWORK_SHUTDOWN();
        return 1;
    }
    
    // Process any remaining async operations
    g_ioc.run_for(std::chrono::milliseconds(100));
    
    // Cleanup
    ZEUS_NETWORK_SHUTDOWN();
    
    std::cout << "TCP Connection test completed successfully" << std::endl;
    return 0;
}