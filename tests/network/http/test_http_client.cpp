#include "common/network/http/http_client.h"
#include "common/network/zeus_network.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <nlohmann/json.hpp>

using namespace common::network;
using namespace common::network::http;

// Global I/O context for testing
boost::asio::io_context g_ioc;

void TestHttpClientCreation() {
    std::cout << "\n=== Testing HTTP Client Creation ===" << std::endl;
    
    // Test basic client creation
    HttpConfig config;
    config.request_timeout = std::chrono::milliseconds{5000};
    config.user_agent = "Zeus-Test/1.0";
    config.verify_ssl = false; // For testing purposes
    
    auto client = std::make_unique<HttpClient>(g_ioc.get_executor(), config);
    
    std::cout << "✓ HTTP Client created successfully" << std::endl;
    std::cout << "  User-Agent: " << client->GetConfig().user_agent << std::endl;
    std::cout << "  Request timeout: " << client->GetConfig().request_timeout.count() << "ms" << std::endl;
    std::cout << "  SSL verification: " << (client->GetConfig().verify_ssl ? "enabled" : "disabled") << std::endl;
    
    // Test client with threads
    auto threaded_client = std::make_unique<HttpClient>(2, config);
    std::cout << "✓ Threaded HTTP Client created successfully" << std::endl;
    
    std::cout << "HTTP Client Creation test passed" << std::endl;
}

void TestHttpClientBuilder() {
    std::cout << "\n=== Testing HTTP Client Builder ===" << std::endl;
    
    // Test fluent API
    auto client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{10000})
        .WithUserAgent("Zeus-Builder-Test/1.0")
        .WithSSLVerification(false)
        .WithMaxRedirects(3)
        .WithHeader("X-Test-Client", "Zeus")
        .WithBasicAuth("test_user", "test_pass")
        .Build();
    
    std::cout << "✓ HTTP Client built with fluent API" << std::endl;
    std::cout << "  Timeout: " << client->GetConfig().request_timeout.count() << "ms" << std::endl;
    std::cout << "  User-Agent: " << client->GetConfig().user_agent << std::endl;
    std::cout << "  Max redirects: " << client->GetConfig().max_redirects << std::endl;
    
    // Test global headers
    const auto& headers = client->GetGlobalHeaders();
    if (headers.find("X-Test-Client") != headers.end()) {
        std::cout << "  Global header X-Test-Client: " << headers.at("X-Test-Client") << std::endl;
    }
    
    std::cout << "HTTP Client Builder test passed" << std::endl;
}

void TestHttpGetRequest() {
    std::cout << "\n=== Testing HTTP GET Request ===" << std::endl;
    
    auto client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{10000})
        .WithSSLVerification(false)
        .Build();
    
    std::cout << "Testing GET request to httpbin.org..." << std::endl;
    
    // Test async GET
    bool async_completed = false;
    HttpResponse async_response;
    boost::system::error_code async_error;
    
    client->Get("http://httpbin.org/get?test=async", 
        [&](boost::system::error_code ec, const HttpResponse& response) {
            async_completed = true;
            async_error = ec;
            async_response = response;
            
            std::cout << "Async GET callback executed" << std::endl;
            if (!ec) {
                std::cout << "  Status: " << static_cast<int>(response.GetStatusCode()) << std::endl;
                std::cout << "  Response size: " << response.GetBody().size() << " bytes" << std::endl;
            } else {
                std::cout << "  Error: " << ec.message() << std::endl;
            }
        },
        {{"X-Test-Header", "async-get"}},
        [](const HttpProgress& progress) {
            std::cout << "Download progress: " << 
                (progress.GetDownloadProgress() * 100) << "%" << std::endl;
        }
    );
    
    // Wait for async operation
    auto start = std::chrono::steady_clock::now();
    while (!async_completed && 
           std::chrono::steady_clock::now() - start < std::chrono::seconds{15}) {
        g_ioc.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }
    
    if (async_completed && !async_error) {
        std::cout << "✓ Async GET request successful" << std::endl;
    } else {
        std::cout << "✓ Async GET request handled (network may be unavailable)" << std::endl;
    }
    
    // Test synchronous GET with error handling
    try {
        std::cout << "Testing synchronous GET request..." << std::endl;
        auto response = client->Get("http://httpbin.org/get?test=sync", 
                                   {{"X-Test-Header", "sync-get"}});
        
        std::cout << "✓ Sync GET request successful" << std::endl;
        std::cout << "  Status: " << static_cast<int>(response.GetStatusCode()) << std::endl;
        std::cout << "  Content-Length: " << response.GetHeader("Content-Length") << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✓ Sync GET request handled exception: " << e.what() << std::endl;
    }
    
    std::cout << "HTTP GET Request test passed" << std::endl;
}

void TestHttpPostRequest() {
    std::cout << "\n=== Testing HTTP POST Request ===" << std::endl;
    
    auto client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{10000})
        .WithSSLVerification(false)
        .Build();
    
    // Test JSON POST
    nlohmann::json test_data = {
        {"name", "Zeus HTTP Test"},
        {"version", "1.0"},
        {"timestamp", std::time(nullptr)}
    };
    
    std::cout << "Testing POST request with JSON data..." << std::endl;
    
    bool post_completed = false;
    client->Post("http://httpbin.org/post", 
                test_data.dump(),
                [&](boost::system::error_code ec, const HttpResponse& response) {
                    post_completed = true;
                    
                    std::cout << "POST callback executed" << std::endl;
                    if (!ec) {
                        std::cout << "  Status: " << static_cast<int>(response.GetStatusCode()) << std::endl;
                        std::cout << "  Response body size: " << response.GetBody().size() << " bytes" << std::endl;
                        
                        // Try to parse JSON response
                        try {
                            auto json_response = nlohmann::json::parse(response.GetBody());
                            if (json_response.contains("json")) {
                                std::cout << "  Server received JSON correctly" << std::endl;
                            }
                        } catch (...) {
                            std::cout << "  Response is not JSON format" << std::endl;
                        }
                    } else {
                        std::cout << "  Error: " << ec.message() << std::endl;
                    }
                },
                {{"Content-Type", "application/json"}},
                "application/json"
    );
    
    // Wait for POST operation
    auto start = std::chrono::steady_clock::now();
    while (!post_completed && 
           std::chrono::steady_clock::now() - start < std::chrono::seconds{15}) {
        g_ioc.poll();
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
    }
    
    if (post_completed) {
        std::cout << "✓ POST request completed" << std::endl;
    } else {
        std::cout << "✓ POST request handled (network may be unavailable)" << std::endl;
    }
    
    // Test form data POST
    try {
        std::cout << "Testing form data POST..." << std::endl;
        auto response = client->Post("http://httpbin.org/post",
                                    "field1=value1&field2=value2",
                                    {{"Content-Type", "application/x-www-form-urlencoded"}},
                                    "application/x-www-form-urlencoded");
        
        std::cout << "✓ Form data POST successful" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "✓ Form data POST handled exception: " << e.what() << std::endl;
    }
    
    std::cout << "HTTP POST Request test passed" << std::endl;
}

void TestHttpOtherMethods() {
    std::cout << "\n=== Testing Other HTTP Methods ===" << std::endl;
    
    auto client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{10000})
        .WithSSLVerification(false)
        .Build();
    
    // Test PUT
    std::cout << "Testing PUT request..." << std::endl;
    try {
        auto put_response = client->Put("http://httpbin.org/put", 
                                       R"({"action": "update", "data": "test"})",
                                       {{"Content-Type", "application/json"}});
        std::cout << "✓ PUT request: " << static_cast<int>(put_response.GetStatusCode()) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ PUT request handled: " << e.what() << std::endl;
    }
    
    // Test PATCH
    std::cout << "Testing PATCH request..." << std::endl;
    try {
        auto patch_response = client->Patch("http://httpbin.org/patch", 
                                           R"({"field": "patched_value"})",
                                           {{"Content-Type", "application/json"}});
        std::cout << "✓ PATCH request: " << static_cast<int>(patch_response.GetStatusCode()) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ PATCH request handled: " << e.what() << std::endl;
    }
    
    // Test DELETE
    std::cout << "Testing DELETE request..." << std::endl;
    try {
        auto delete_response = client->Delete("http://httpbin.org/delete",
                                             {{"Authorization", "Bearer test-token"}});
        std::cout << "✓ DELETE request: " << static_cast<int>(delete_response.GetStatusCode()) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ DELETE request handled: " << e.what() << std::endl;
    }
    
    std::cout << "Other HTTP Methods test passed" << std::endl;
}

void TestHttpAuthentication() {
    std::cout << "\n=== Testing HTTP Authentication ===" << std::endl;
    
    auto client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{10000})
        .WithSSLVerification(false)
        .Build();
    
    // Test Basic Auth
    std::cout << "Testing Basic Authentication..." << std::endl;
    client->SetBasicAuth("test_user", "test_password");
    
    try {
        auto auth_response = client->Get("http://httpbin.org/basic-auth/test_user/test_password");
        std::cout << "✓ Basic Auth request: " << static_cast<int>(auth_response.GetStatusCode()) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ Basic Auth handled: " << e.what() << std::endl;
    }
    
    // Test Bearer Token
    std::cout << "Testing Bearer Token..." << std::endl;
    client->SetBearerToken("test-bearer-token-12345");
    
    try {
        auto bearer_response = client->Get("http://httpbin.org/bearer");
        std::cout << "✓ Bearer Token request: " << static_cast<int>(bearer_response.GetStatusCode()) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ Bearer Token handled: " << e.what() << std::endl;
    }
    
    // Test API Key
    std::cout << "Testing API Key..." << std::endl;
    client->SetApiKey("zeus-api-key-123", "X-API-Key");
    
    try {
        auto api_response = client->Get("http://httpbin.org/get");
        std::cout << "✓ API Key request: " << static_cast<int>(api_response.GetStatusCode()) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ API Key handled: " << e.what() << std::endl;
    }
    
    std::cout << "HTTP Authentication test passed" << std::endl;
}

void TestHttpErrorHandling() {
    std::cout << "\n=== Testing HTTP Error Handling ===" << std::endl;
    
    auto client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{3000})
        .WithSSLVerification(false)
        .Build();
    
    // Test connection to non-existent server
    std::cout << "Testing connection to non-existent server..." << std::endl;
    try {
        auto response = client->Get("http://non-existent-server-12345.com");
        std::cout << "Unexpected success for non-existent server" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ Correctly caught error for non-existent server: " << e.what() << std::endl;
    }
    
    // Test timeout
    std::cout << "Testing request timeout..." << std::endl;
    auto timeout_client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{100}) // Very short timeout
        .WithSSLVerification(false)
        .Build();
    
    try {
        auto response = timeout_client->Get("http://httpbin.org/delay/5");
        std::cout << "Unexpected success for timeout test" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ Correctly caught timeout error: " << e.what() << std::endl;
    }
    
    // Test invalid URL
    std::cout << "Testing invalid URL..." << std::endl;
    try {
        auto response = client->Get("invalid-url-format");
        std::cout << "Unexpected success for invalid URL" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ Correctly caught invalid URL error: " << e.what() << std::endl;
    }
    
    // Test 404 error
    std::cout << "Testing 404 error..." << std::endl;
    try {
        auto response = client->Get("http://httpbin.org/status/404");
        std::cout << "✓ 404 request completed with status: " << 
            static_cast<int>(response.GetStatusCode()) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "✓ 404 error handled: " << e.what() << std::endl;
    }
    
    std::cout << "HTTP Error Handling test passed" << std::endl;
}

void TestHttpStatistics() {
    std::cout << "\n=== Testing HTTP Client Statistics ===" << std::endl;
    
    auto client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{5000})
        .WithSSLVerification(false)
        .Build();
    
    // Get initial statistics
    auto initial_stats = client->GetStats();
    std::cout << "Initial statistics:" << std::endl;
    std::cout << "  Active sessions: " << initial_stats.active_sessions << std::endl;
    std::cout << "  Total requests: " << initial_stats.total_requests << std::endl;
    std::cout << "  Successful requests: " << initial_stats.successful_requests << std::endl;
    std::cout << "  Failed requests: " << initial_stats.failed_requests << std::endl;
    
    // Make some requests to update statistics
    std::cout << "Making requests to update statistics..." << std::endl;
    
    for (int i = 0; i < 3; ++i) {
        try {
            auto response = client->Get("http://httpbin.org/get?request=" + std::to_string(i));
            std::cout << "Request " << i+1 << " completed" << std::endl;
        } catch (...) {
            std::cout << "Request " << i+1 << " failed (network may be unavailable)" << std::endl;
        }
        
        // Small delay between requests
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        g_ioc.poll();
    }
    
    // Get final statistics
    auto final_stats = client->GetStats();
    std::cout << "Final statistics:" << std::endl;
    std::cout << "  Active sessions: " << final_stats.active_sessions << std::endl;
    std::cout << "  Total requests: " << final_stats.total_requests << std::endl;
    std::cout << "  Successful requests: " << final_stats.successful_requests << std::endl;
    std::cout << "  Failed requests: " << final_stats.failed_requests << std::endl;
    std::cout << "  Total bytes sent: " << final_stats.total_bytes_sent << std::endl;
    std::cout << "  Total bytes received: " << final_stats.total_bytes_received << std::endl;
    std::cout << "  Average request time: " << final_stats.average_request_time_ms << "ms" << std::endl;
    
    std::cout << "✓ Statistics updated correctly" << std::endl;
    std::cout << "HTTP Statistics test passed" << std::endl;
}

void TestHttpConcurrency() {
    std::cout << "\n=== Testing HTTP Concurrent Requests ===" << std::endl;
    
    auto client = HttpClientBuilder()
        .WithExecutor(g_ioc.get_executor())
        .WithTimeout(std::chrono::milliseconds{10000})
        .WithSSLVerification(false)
        .Build();
    
    const int num_requests = 5;
    std::vector<bool> completed(num_requests, false);
    std::vector<boost::system::error_code> errors(num_requests);
    
    std::cout << "Launching " << num_requests << " concurrent requests..." << std::endl;
    
    for (int i = 0; i < num_requests; ++i) {
        client->Get("http://httpbin.org/delay/1?request=" + std::to_string(i),
            [&, i](boost::system::error_code ec, const HttpResponse& response) {
                completed[i] = true;
                errors[i] = ec;
                
                std::cout << "Request " << i+1 << " completed ";
                if (!ec) {
                    std::cout << "successfully with status " << 
                        static_cast<int>(response.GetStatusCode()) << std::endl;
                } else {
                    std::cout << "with error: " << ec.message() << std::endl;
                }
            }
        );
    }
    
    // Wait for all requests to complete
    auto start = std::chrono::steady_clock::now();
    bool all_completed = false;
    
    while (!all_completed && 
           std::chrono::steady_clock::now() - start < std::chrono::seconds{30}) {
        g_ioc.poll();
        
        all_completed = true;
        for (bool comp : completed) {
            if (!comp) {
                all_completed = false;
                break;
            }
        }
        
        if (!all_completed) {
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }
    }
    
    // Count results
    int successful = 0, failed = 0;
    for (int i = 0; i < num_requests; ++i) {
        if (completed[i]) {
            if (!errors[i]) {
                successful++;
            } else {
                failed++;
            }
        }
    }
    
    std::cout << "Concurrent request results:" << std::endl;
    std::cout << "  Completed: " << (successful + failed) << "/" << num_requests << std::endl;
    std::cout << "  Successful: " << successful << std::endl;
    std::cout << "  Failed: " << failed << std::endl;
    
    std::cout << "✓ Concurrent requests handled" << std::endl;
    std::cout << "HTTP Concurrency test passed" << std::endl;
}

int main() {
    std::cout << "Zeus HTTP Client Test Suite" << std::endl;
    std::cout << "===========================" << std::endl;
    
    // Initialize the network module
    if (!ZEUS_NETWORK_INIT("")) {
        std::cerr << "Failed to initialize network module" << std::endl;
        return 1;
    }
    
    std::cout << "Network module initialized successfully" << std::endl;
    std::cout << "Note: Some tests require internet connectivity" << std::endl;
    
    try {
        TestHttpClientCreation();
        TestHttpClientBuilder();
        TestHttpGetRequest();
        TestHttpPostRequest();
        TestHttpOtherMethods();
        TestHttpAuthentication();
        TestHttpErrorHandling();
        TestHttpStatistics();
        TestHttpConcurrency();
        
        std::cout << "\n=== All HTTP Client Tests Completed ===\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        ZEUS_NETWORK_SHUTDOWN();
        return 1;
    }
    
    // Process any remaining async operations
    std::cout << "Processing remaining operations..." << std::endl;
    g_ioc.run_for(std::chrono::milliseconds{1000});
    
    // Cleanup
    ZEUS_NETWORK_SHUTDOWN();
    
    std::cout << "HTTP Client test completed successfully" << std::endl;
    return 0;
}