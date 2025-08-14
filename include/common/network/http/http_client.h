#pragma once

#include "http_message.h"
#include "../network_logger.h"
#include "../network_events.h"
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <future>

namespace common {
namespace network {
namespace http {

/**
 * @brief HTTP client session for handling individual requests
 */
class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    /**
     * @brief Constructor
     * @param executor ASIO executor
     * @param config HTTP configuration
     */
    explicit HttpSession(boost::asio::any_io_executor executor, const HttpConfig& config = HttpConfig{});
    
    /**
     * @brief Destructor
     */
    ~HttpSession();
    
    /**
     * @brief Execute HTTP request asynchronously
     * @param request HTTP request to send
     * @param callback Callback for response or error
     * @param progress_callback Optional progress callback
     */
    void AsyncRequest(const HttpRequest& request, 
                     HttpResponseCallback callback,
                     HttpProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Execute HTTP request synchronously
     * @param request HTTP request to send
     * @param timeout_ms Request timeout in milliseconds
     * @return HTTP response
     * @throws HttpException on error
     */
    HttpResponse Request(const HttpRequest& request, 
                        std::chrono::milliseconds timeout_ms = std::chrono::milliseconds{30000});
    
    /**
     * @brief Cancel ongoing request
     */
    void Cancel();
    
    /**
     * @brief Check if session is busy
     */
    bool IsBusy() const { return busy_; }
    
    /**
     * @brief Get session configuration
     */
    const HttpConfig& GetConfig() const { return config_; }
    
    /**
     * @brief Update session configuration
     */
    void SetConfig(const HttpConfig& config) { config_ = config; }

private:
    // Connection management
    void Connect(const HttpUrl& url, std::function<void(boost::system::error_code)> callback);
    void Disconnect();
    bool IsConnected() const;
    
    // Request processing
    void DoRequest(const HttpRequest& request, HttpResponseCallback callback, HttpProgressCallback progress_callback);
    void WriteRequest(const HttpRequest& request);
    void ReadResponse(HttpResponseCallback callback);
    void HandleRedirect(const HttpResponse& response, const HttpRequest& original_request, 
                       HttpResponseCallback callback, HttpProgressCallback progress_callback, size_t redirect_count);
    
    // SSL/TLS support
    void SetupSSL(const HttpUrl& url);
    void PerformSSLHandshake(std::function<void(boost::system::error_code)> callback);
    
    // Timeout handling
    void StartTimeout(std::chrono::milliseconds timeout);
    void CancelTimeout();
    
    // Error handling
    void HandleError(boost::system::error_code ec, const std::string& operation);
    void FireNetworkEvent(NetworkEventType type, const std::string& details = "");
    
    boost::asio::any_io_executor executor_;
    HttpConfig config_;
    
    // Network components
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> ssl_stream_;
    std::unique_ptr<boost::asio::ssl::context> ssl_context_;
    boost::asio::ip::tcp::resolver resolver_;
    
    // Request/Response handling
    boost::beast::flat_buffer buffer_;
    boost::beast::http::request<boost::beast::http::string_body> beast_request_;
    boost::beast::http::response<boost::beast::http::string_body> beast_response_;
    
    // State management
    std::atomic<bool> busy_{false};
    std::atomic<bool> cancelled_{false};
    HttpUrl current_url_;
    std::string session_id_;
    
    // Timeout handling
    std::unique_ptr<boost::asio::steady_timer> timeout_timer_;
    
    // Statistics
    std::chrono::steady_clock::time_point request_start_time_;
    size_t bytes_sent_ = 0;
    size_t bytes_received_ = 0;
};

/**
 * @brief HTTP client pool for managing multiple sessions
 */
class HttpClient {
public:
    /**
     * @brief Constructor
     * @param executor ASIO executor
     * @param config Default HTTP configuration
     */
    explicit HttpClient(boost::asio::any_io_executor executor, const HttpConfig& config = HttpConfig{});
    
    /**
     * @brief Constructor with thread pool
     * @param thread_count Number of threads in pool
     * @param config Default HTTP configuration
     */
    explicit HttpClient(size_t thread_count, const HttpConfig& config = HttpConfig{});
    
    /**
     * @brief Destructor
     */
    ~HttpClient();
    
    // Async Request Methods
    
    /**
     * @brief Execute GET request asynchronously
     */
    void Get(const std::string& url, HttpResponseCallback callback, 
             const HttpHeaders& headers = {}, HttpProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Execute POST request asynchronously
     */
    void Post(const std::string& url, const std::string& body, HttpResponseCallback callback,
              const HttpHeaders& headers = {}, const std::string& content_type = "text/plain",
              HttpProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Execute PUT request asynchronously
     */
    void Put(const std::string& url, const std::string& body, HttpResponseCallback callback,
             const HttpHeaders& headers = {}, const std::string& content_type = "text/plain",
             HttpProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Execute DELETE request asynchronously
     */
    void Delete(const std::string& url, HttpResponseCallback callback, 
                const HttpHeaders& headers = {}, HttpProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Execute PATCH request asynchronously
     */
    void Patch(const std::string& url, const std::string& body, HttpResponseCallback callback,
               const HttpHeaders& headers = {}, const std::string& content_type = "text/plain",
               HttpProgressCallback progress_callback = nullptr);
    
    /**
     * @brief Execute custom request asynchronously
     */
    void Request(const HttpRequest& request, HttpResponseCallback callback, 
                HttpProgressCallback progress_callback = nullptr);
    
    // Synchronous Request Methods
    
    /**
     * @brief Execute GET request synchronously
     */
    HttpResponse Get(const std::string& url, const HttpHeaders& headers = {},
                    std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});
    
    /**
     * @brief Execute POST request synchronously
     */
    HttpResponse Post(const std::string& url, const std::string& body,
                     const HttpHeaders& headers = {}, const std::string& content_type = "text/plain",
                     std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});
    
    /**
     * @brief Execute PUT request synchronously
     */
    HttpResponse Put(const std::string& url, const std::string& body,
                    const HttpHeaders& headers = {}, const std::string& content_type = "text/plain",
                    std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});
    
    /**
     * @brief Execute DELETE request synchronously
     */
    HttpResponse Delete(const std::string& url, const HttpHeaders& headers = {},
                       std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});
    
    /**
     * @brief Execute PATCH request synchronously
     */
    HttpResponse Patch(const std::string& url, const std::string& body,
                      const HttpHeaders& headers = {}, const std::string& content_type = "text/plain",
                      std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});
    
    /**
     * @brief Execute custom request synchronously
     */
    HttpResponse Request(const HttpRequest& request, 
                        std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});
    
    // JSON Convenience Methods
    
    /**
     * @brief Execute GET request for JSON response
     */
    std::future<nlohmann::json> GetJson(const std::string& url, const HttpHeaders& headers = {});
    
    /**
     * @brief Execute POST request with JSON body
     */
    std::future<nlohmann::json> PostJson(const std::string& url, const nlohmann::json& json, 
                                        const HttpHeaders& headers = {});
    
    /**
     * @brief Execute PUT request with JSON body
     */
    std::future<nlohmann::json> PutJson(const std::string& url, const nlohmann::json& json, 
                                       const HttpHeaders& headers = {});
    
    /**
     * @brief Execute PATCH request with JSON body
     */
    std::future<nlohmann::json> PatchJson(const std::string& url, const nlohmann::json& json, 
                                         const HttpHeaders& headers = {});
    
    // File Operations
    
    /**
     * @brief Download file asynchronously
     */
    void DownloadFile(const std::string& url, const std::string& local_path, 
                     std::function<void(boost::system::error_code, const std::string&)> callback,
                     HttpProgressCallback progress_callback = nullptr, const HttpHeaders& headers = {});
    
    /**
     * @brief Upload file asynchronously
     */
    void UploadFile(const std::string& url, const std::string& file_path, const std::string& field_name,
                   HttpResponseCallback callback, HttpProgressCallback progress_callback = nullptr,
                   const HttpHeaders& headers = {});
    
    // Session Management
    
    /**
     * @brief Set global headers for all requests
     */
    void SetGlobalHeaders(const HttpHeaders& headers) { global_headers_ = headers; }
    
    /**
     * @brief Add global header
     */
    void SetGlobalHeader(const std::string& name, const std::string& value) { global_headers_[name] = value; }
    
    /**
     * @brief Remove global header
     */
    void RemoveGlobalHeader(const std::string& name) { global_headers_.erase(name); }
    
    /**
     * @brief Get global headers
     */
    const HttpHeaders& GetGlobalHeaders() const { return global_headers_; }
    
    /**
     * @brief Set global cookies
     */
    void SetGlobalCookies(const std::vector<HttpCookie>& cookies) { global_cookies_ = cookies; }
    
    /**
     * @brief Add global cookie
     */
    void AddGlobalCookie(const HttpCookie& cookie) { global_cookies_.push_back(cookie); }
    
    /**
     * @brief Get global cookies
     */
    const std::vector<HttpCookie>& GetGlobalCookies() const { return global_cookies_; }
    
    /**
     * @brief Clear all cookies
     */
    void ClearCookies() { global_cookies_.clear(); }
    
    /**
     * @brief Set authentication for all requests
     */
    void SetBasicAuth(const std::string& username, const std::string& password);
    void SetBearerToken(const std::string& token);
    void SetApiKey(const std::string& key, const std::string& header_name = "X-API-Key");
    
    /**
     * @brief Get/Set configuration
     */
    const HttpConfig& GetConfig() const { return config_; }
    void SetConfig(const HttpConfig& config) { config_ = config; }
    
    /**
     * @brief Get client statistics
     */
    struct ClientStats {
        size_t active_sessions = 0;
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        size_t total_bytes_sent = 0;
        size_t total_bytes_received = 0;
        std::chrono::steady_clock::time_point created_at;
        double average_request_time_ms = 0.0;
        
        ClientStats() : created_at(std::chrono::steady_clock::now()) {}
    };
    
    /**
     * @brief Get current client statistics
     */
    ClientStats GetStats() const;
    
    /**
     * @brief Cancel all pending requests
     */
    void CancelAllRequests();
    
    /**
     * @brief Wait for all requests to complete
     */
    void WaitForCompletion(std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});

private:
    // Session pool management
    std::shared_ptr<HttpSession> GetAvailableSession();
    void ReturnSession(std::shared_ptr<HttpSession> session);
    void CreateSession();
    void CleanupSessions();
    
    // Request preparation
    HttpRequest PrepareRequest(const HttpRequest& original_request) const;
    
    // Statistics update
    void UpdateStats(bool success, std::chrono::milliseconds duration, size_t bytes_sent, size_t bytes_received);
    
    boost::asio::any_io_executor executor_;
    HttpConfig config_;
    
    // Session pool
    std::vector<std::shared_ptr<HttpSession>> session_pool_;
    std::queue<std::shared_ptr<HttpSession>> available_sessions_;
    std::mutex session_mutex_;
    std::condition_variable session_cv_;
    size_t max_pool_size_ = 10;
    
    // Global settings
    HttpHeaders global_headers_;
    std::vector<HttpCookie> global_cookies_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    ClientStats stats_;
    
    // Thread management (for constructor with thread_count)
    std::unique_ptr<boost::asio::io_context> owned_ioc_;
    std::vector<std::thread> worker_threads_;
    
    // Lifecycle management
    std::atomic<bool> shutdown_requested_{false};
};

/**
 * @brief HTTP client builder for fluent API
 */
class HttpClientBuilder {
public:
    HttpClientBuilder() = default;
    
    /**
     * @brief Set executor
     */
    HttpClientBuilder& WithExecutor(boost::asio::any_io_executor executor) {
        executor_ = executor;
        return *this;
    }
    
    /**
     * @brief Set thread count (creates internal io_context)
     */
    HttpClientBuilder& WithThreads(size_t thread_count) {
        thread_count_ = thread_count;
        return *this;
    }
    
    /**
     * @brief Set timeout
     */
    HttpClientBuilder& WithTimeout(std::chrono::milliseconds timeout) {
        config_.request_timeout = timeout;
        return *this;
    }
    
    /**
     * @brief Set User-Agent
     */
    HttpClientBuilder& WithUserAgent(const std::string& user_agent) {
        config_.user_agent = user_agent;
        return *this;
    }
    
    /**
     * @brief Enable/disable SSL verification
     */
    HttpClientBuilder& WithSSLVerification(bool verify) {
        config_.verify_ssl = verify;
        return *this;
    }
    
    /**
     * @brief Set maximum redirects
     */
    HttpClientBuilder& WithMaxRedirects(size_t max_redirects) {
        config_.max_redirects = max_redirects;
        return *this;
    }
    
    /**
     * @brief Set basic authentication
     */
    HttpClientBuilder& WithBasicAuth(const std::string& username, const std::string& password) {
        basic_auth_username_ = username;
        basic_auth_password_ = password;
        return *this;
    }
    
    /**
     * @brief Set bearer token
     */
    HttpClientBuilder& WithBearerToken(const std::string& token) {
        bearer_token_ = token;
        return *this;
    }
    
    /**
     * @brief Set API key
     */
    HttpClientBuilder& WithApiKey(const std::string& key, const std::string& header_name = "X-API-Key") {
        api_key_ = key;
        api_key_header_ = header_name;
        return *this;
    }
    
    /**
     * @brief Add global header
     */
    HttpClientBuilder& WithHeader(const std::string& name, const std::string& value) {
        headers_[name] = value;
        return *this;
    }
    
    /**
     * @brief Add global cookie
     */
    HttpClientBuilder& WithCookie(const std::string& name, const std::string& value) {
        cookies_.emplace_back(name, value);
        return *this;
    }
    
    /**
     * @brief Build the HTTP client
     */
    std::unique_ptr<HttpClient> Build();

private:
    std::optional<boost::asio::any_io_executor> executor_;
    size_t thread_count_ = 1;
    HttpConfig config_;
    HttpHeaders headers_;
    std::vector<HttpCookie> cookies_;
    std::string basic_auth_username_;
    std::string basic_auth_password_;
    std::string bearer_token_;
    std::string api_key_;
    std::string api_key_header_ = "X-API-Key";
};

} // namespace http
} // namespace network
} // namespace common