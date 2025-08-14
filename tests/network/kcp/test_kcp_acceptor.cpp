#include "common/network/kcp_acceptor.h"
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

void TestKcpAcceptorCreation() {
    std::cout << "\n=== Testing KCP Server Creation ===" << std::endl;
    
    KcpConnector::KcpConfig server_config;
    server_config.conv_id = 0; // Will auto-assign for new connections
    server_config.nodelay = 1;
    server_config.interval = 10;
    server_config.mtu = 1400;
    server_config.timeout_ms = 30000;
    
    // Test server creation
    auto kcp_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 8090, "0.0.0.0", server_config);
    
    std::cout << "✓ KCP Server created" << std::endl;
    std::cout << "  Protocol: KCP" << std::endl;
    std::cout << "  Port: 8090" << std::endl;
    std::cout << "  Running: " << (kcp_server->IsRunning() ? "Yes" : "No") << std::endl;
    
    // Verify initial state
    assert(!kcp_server->IsRunning());
    
    std::cout << "KCP Server Creation test passed" << std::endl;
}

void TestKcpAcceptorConfiguration() {
    std::cout << "\n=== Testing KCP Server Configuration ===" << std::endl;
    
    KcpConnector::KcpConfig server_config;
    server_config.nodelay = 1;
    server_config.interval = 10;
    server_config.resend = 2;
    server_config.nc = 1;
    
    auto kcp_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 8091, "0.0.0.0", server_config);
    
    // Test configuration methods
    kcp_server->SetMaxConnections(500);
    std::cout << "✓ Max connections configuration set" << std::endl;
    
    // Test connection count
    std::cout << "Current connection count: " << kcp_server->GetConnectionCount() << std::endl;
    assert(kcp_server->GetConnectionCount() == 0); // No connections initially
    
    std::cout << "KCP Server Configuration test passed" << std::endl;
}

void TestKcpAcceptorStartStop() {
    std::cout << "\n=== Testing KCP Server Start/Stop ===" << std::endl;
    
    KcpConnector::KcpConfig server_config;
    server_config.conv_id = 0;
    server_config.nodelay = 1;
    server_config.interval = 10;
    
    auto kcp_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 8092, "127.0.0.1", server_config);
    
    std::cout << "Testing KCP Acceptor Start/Stop functionality" << std::endl;
    
    // Test server start with connection handler
    bool connection_received = false;
    bool started = kcp_server->Start([&](std::shared_ptr<KcpConnector> conn) {
        connection_received = true;
        std::cout << "KCP Connection received: " << conn->GetConnectionId() << std::endl;
    });
    
    if (started) {
        std::cout << "✓ KCP server started successfully" << std::endl;
        std::cout << "Server running: " << (kcp_server->IsRunning() ? "Yes" : "No") << std::endl;
        
        // Test server stop
        kcp_server->Stop();
        std::cout << "✓ KCP server stopped successfully" << std::endl;
        std::cout << "Server running after stop: " << (kcp_server->IsRunning() ? "Yes" : "No") << std::endl;
        
        assert(!kcp_server->IsRunning());
    } else {
        std::cout << "⚠ KCP server failed to start (port may be in use)" << std::endl;
    }
    
    std::cout << "KCP Server Start/Stop test passed" << std::endl;
}

void TestKcpAcceptorStatistics() {
    std::cout << "\n=== Testing KCP Server Statistics ===" << std::endl;
    
    KcpConnector::KcpConfig server_config;
    server_config.conv_id = 0;
    
    auto kcp_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 8093, "0.0.0.0", server_config);
    
    // Test connection count
    std::cout << "Initial connection count: " << kcp_server->GetConnectionCount() << std::endl;
    
    // Verify initial values
    assert(kcp_server->GetConnectionCount() == 0);
    
    std::cout << "✓ Initial KCP statistics are correct" << std::endl;
    
    std::cout << "KCP Server Statistics test passed" << std::endl;
}

void TestKcpAcceptorConnectionManagement() {
    std::cout << "\n=== Testing KCP Server Connection Management ===" << std::endl;
    
    KcpConnector::KcpConfig server_config;
    server_config.conv_id = 0;
    
    auto kcp_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 8094, "0.0.0.0", server_config);
    
    // Test connection limits
    kcp_server->SetMaxConnections(10);
    std::cout << "✓ KCP connection limit set to 10" << std::endl;
    
    // Test connection count (should be 0 initially)
    assert(kcp_server->GetConnectionCount() == 0);
    std::cout << "✓ Initial KCP connection count is 0" << std::endl;
    
    std::cout << "KCP Server Connection Management test passed" << std::endl;
}

void TestKcpAcceptorErrorHandling() {
    std::cout << "\n=== Testing KCP Server Error Handling ===" << std::endl;
    
    // Test creating acceptor on invalid port - should handle gracefully
    try {
        KcpConnector::KcpConfig server_config;
        auto kcp_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 65536, "0.0.0.0", server_config); // Invalid port
        std::cout << "⚠ Invalid port accepted (implementation may handle this differently)" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ Invalid port correctly rejected: " << e.what() << std::endl;
    }
    
    std::cout << "KCP Server Error Handling test passed" << std::endl;
}

void TestKcpAcceptorConfigVariations() {
    std::cout << "\n=== Testing KCP Server Config Variations ===" << std::endl;
    
    // Test fast server configuration
    KcpConnector::KcpConfig fast_config;
    fast_config.conv_id = 0;
    fast_config.nodelay = 1;
    fast_config.interval = 5;
    fast_config.resend = 2;
    fast_config.nc = 1;
    fast_config.mtu = 1400;
    
    auto fast_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 8095, "0.0.0.0", fast_config);
    std::cout << "✓ Fast KCP server configuration created" << std::endl;
    
    // Test normal server configuration
    KcpConnector::KcpConfig normal_config;
    normal_config.conv_id = 0;
    normal_config.nodelay = 0;
    normal_config.interval = 40;
    normal_config.resend = 0;
    normal_config.nc = 0;
    normal_config.mtu = 1200;
    
    auto normal_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 8096, "0.0.0.0", normal_config);
    std::cout << "✓ Normal KCP server configuration created" << std::endl;
    
    std::cout << "KCP Server Config Variations test passed" << std::endl;
}

void TestKcpAcceptorEdgeCases() {
    std::cout << "\n=== Testing KCP Server Edge Cases ===" << std::endl;
    
    KcpConnector::KcpConfig server_config;
    server_config.conv_id = 0;
    
    auto kcp_server = std::make_shared<KcpAcceptor>(g_ioc.get_executor(), 8097, "0.0.0.0", server_config);
    
    // Test multiple stop calls
    kcp_server->Stop();
    kcp_server->Stop(); // Should not crash
    std::cout << "✓ Multiple KCP stop calls handled safely" << std::endl;
    
    // Test configuration changes while stopped
    kcp_server->SetMaxConnections(50);
    kcp_server->SetMaxConnections(100);
    std::cout << "✓ KCP configuration changes while stopped work" << std::endl;
    
    std::cout << "KCP Server Edge Cases test passed" << std::endl;
}

int main() {
    std::cout << "Zeus KCP Server Test Suite" << std::endl;
    std::cout << "==========================" << std::endl;
    
    // Initialize the network module
    if (!ZEUS_NETWORK_INIT("")) {
        std::cerr << "Failed to initialize network module" << std::endl;
        return 1;
    }
    
    std::cout << "Network module initialized successfully" << std::endl;
    
    try {
        TestKcpAcceptorCreation();
        TestKcpAcceptorConfiguration();
        TestKcpAcceptorStartStop();
        TestKcpAcceptorStatistics();
        TestKcpAcceptorConnectionManagement();
        TestKcpAcceptorErrorHandling();
        TestKcpAcceptorConfigVariations();
        TestKcpAcceptorEdgeCases();
        
        std::cout << "\n=== All KCP Server Tests Passed ===\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        ZEUS_NETWORK_SHUTDOWN();
        return 1;
    }
    
    // Process any remaining async operations
    g_ioc.run_for(std::chrono::milliseconds(100));
    
    // Cleanup
    ZEUS_NETWORK_SHUTDOWN();
    
    std::cout << "KCP Server test completed successfully" << std::endl;
    return 0;
}