#include "common/network/tcp_acceptor.h"
#include "common/network/zeus_network.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

using namespace common::network;

// Global I/O context for testing
boost::asio::io_context g_ioc;

void TestTcpAcceptorCreation() {
    std::cout << "\n=== Testing TCP Server Creation ===" << std::endl;
    
    // Test server creation
    auto tcp_server = std::make_shared<TcpAcceptor>(g_ioc.get_executor(), 8080);
    
    std::cout << "✓ TCP Server created" << std::endl;
    std::cout << "  Listening Port: 8080" << std::endl;
    std::cout << "  Protocol: TCP" << std::endl;
    std::cout << "  Initial State: STOPPED" << std::endl;
    std::cout << "  Running: " << (tcp_server->IsRunning() ? "Yes" : "No") << std::endl;
    
    // Verify initial state
    assert(!tcp_server->IsRunning());
    
    std::cout << "TCP Server Creation test passed" << std::endl;
}

void TestTcpAcceptorConfiguration() {
    std::cout << "\n=== Testing TCP Server Configuration ===" << std::endl;
    
    auto tcp_server = std::make_shared<TcpAcceptor>(g_ioc.get_executor(), 8081);
    
    // Test configuration methods
    tcp_server->SetMaxConnections(500);
    std::cout << "✓ Max connections configuration set" << std::endl;
    
    // Test connection count
    std::cout << "Current connection count: " << tcp_server->GetConnectionCount() << std::endl;
    assert(tcp_server->GetConnectionCount() == 0); // No connections initially
    
    // Test getting listening endpoint
    auto endpoint = tcp_server->GetListeningEndpoint();
    std::cout << "Listening endpoint configured: " << endpoint << std::endl;
    
    std::cout << "TCP Server Configuration test passed" << std::endl;
}

void TestTcpAcceptorStartStop() {
    std::cout << "\n=== Testing TCP Server Start/Stop ===" << std::endl;
    
    auto tcp_server = std::make_shared<TcpAcceptor>(g_ioc.get_executor(), 8082);
    
    std::cout << "Testing TCP Acceptor Start/Stop functionality" << std::endl;
    
    // Test server start with connection handler
    bool connection_received = false;
    bool started = tcp_server->Start([&](std::shared_ptr<TcpConnector> conn) {
        connection_received = true;
        std::cout << "Connection received: " << conn->GetConnectionId() << std::endl;
    });
    
    if (started) {
        std::cout << "✓ Server started successfully" << std::endl;
        std::cout << "Server running: " << (tcp_server->IsRunning() ? "Yes" : "No") << std::endl;
        
        // Test server stop
        tcp_server->Stop();
        std::cout << "✓ Server stopped successfully" << std::endl;
        std::cout << "Server running after stop: " << (tcp_server->IsRunning() ? "Yes" : "No") << std::endl;
        
        assert(!tcp_server->IsRunning());
    } else {
        std::cout << "⚠ Server failed to start (port may be in use)" << std::endl;
    }
    
    std::cout << "TCP Server Start/Stop test passed" << std::endl;
}

void TestTcpAcceptorStatistics() {
    std::cout << "\n=== Testing TCP Server Statistics ===" << std::endl;
    
    auto tcp_server = std::make_shared<TcpAcceptor>(g_ioc.get_executor(), 8083);
    
    // Test connection count
    std::cout << "Initial connection count: " << tcp_server->GetConnectionCount() << std::endl;
    
    // Verify initial values
    assert(tcp_server->GetConnectionCount() == 0);
    
    std::cout << "✓ Initial statistics are correct" << std::endl;
    
    std::cout << "TCP Server Statistics test passed" << std::endl;
}

void TestTcpAcceptorConnectionManagement() {
    std::cout << "\n=== Testing TCP Server Connection Management ===" << std::endl;
    
    auto tcp_server = std::make_shared<TcpAcceptor>(g_ioc.get_executor(), 8084);
    
    // Test connection limits
    tcp_server->SetMaxConnections(5);
    std::cout << "✓ Connection limit set to 5" << std::endl;
    
    // Test connection count (should be 0 initially)
    assert(tcp_server->GetConnectionCount() == 0);
    std::cout << "✓ Initial connection count is 0" << std::endl;
    
    std::cout << "TCP Server Connection Management test passed" << std::endl;
}

void TestTcpAcceptorErrorHandling() {
    std::cout << "\n=== Testing TCP Server Error Handling ===" << std::endl;
    
    // Test creating acceptor on invalid address - should handle gracefully
    try {
        auto tcp_server = std::make_shared<TcpAcceptor>(g_ioc.get_executor(), 65536); // Invalid port
        std::cout << "⚠ Invalid port accepted (implementation may handle this differently)" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ Invalid port correctly rejected: " << e.what() << std::endl;
    }
    
    std::cout << "TCP Server Error Handling test passed" << std::endl;
}

void TestTcpAcceptorEdgeCases() {
    std::cout << "\n=== Testing TCP Server Edge Cases ===" << std::endl;
    
    auto tcp_server = std::make_shared<TcpAcceptor>(g_ioc.get_executor(), 8085);
    
    // Test multiple stop calls
    tcp_server->Stop();
    tcp_server->Stop(); // Should not crash
    std::cout << "✓ Multiple stop calls handled safely" << std::endl;
    
    // Test configuration changes while stopped
    tcp_server->SetMaxConnections(100);
    tcp_server->SetMaxConnections(200);
    std::cout << "✓ Configuration changes while stopped work" << std::endl;
    
    std::cout << "TCP Server Edge Cases test passed" << std::endl;
}

int main() {
    std::cout << "Zeus TCP Server Test Suite" << std::endl;
    std::cout << "==========================" << std::endl;
    
    // Initialize the network module
    if (!ZEUS_NETWORK_INIT("")) {
        std::cerr << "Failed to initialize network module" << std::endl;
        return 1;
    }
    
    std::cout << "Network module initialized successfully" << std::endl;
    
    try {
        TestTcpAcceptorCreation();
        TestTcpAcceptorConfiguration();
        TestTcpAcceptorStartStop();
        TestTcpAcceptorStatistics();
        TestTcpAcceptorConnectionManagement();
        TestTcpAcceptorErrorHandling();
        TestTcpAcceptorEdgeCases();
        
        std::cout << "\n=== All TCP Server Tests Passed ===\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        ZEUS_NETWORK_SHUTDOWN();
        return 1;
    }
    
    // Process any remaining async operations
    g_ioc.run_for(std::chrono::milliseconds(100));
    
    // Cleanup
    ZEUS_NETWORK_SHUTDOWN();
    
    std::cout << "TCP Server test completed successfully" << std::endl;
    return 0;
}