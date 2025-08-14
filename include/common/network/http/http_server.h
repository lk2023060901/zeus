#pragma once

#include "http_message.h"
#include "http_common.h"
#include "../network_logger.h"
#include "../network_events.h"
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <memory>
#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <regex>

namespace common {
namespace network {
namespace http {

// Forward declarations
class HttpServer;
class HttpRouter;

/**
 * @brief HTTP服务端会话类，处理单个客户端连接
 */
class HttpServerSession : public std::enable_shared_from_this<HttpServerSession> {
public:
    /**
     * @brief 构造函数
     * @param socket TCP socket
     * @param server HTTP服务器引用
     */
    explicit HttpServerSession(boost::asio::ip::tcp::socket socket, HttpServer& server);
    
    // SSL constructor
    explicit HttpServerSession(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream, HttpServer& server);
    
    /**
     * @brief 析构函数
     */
    ~HttpServerSession();
    
    /**
     * @brief 开始处理请求
     */
    void Start();
    
    /**
     * @brief 关闭会话
     */
    void Close();
    
    /**
     * @brief 获取远程端点信息
     */
    std::string GetRemoteEndpoint() const;
    
    /**
     * @brief 获取本地端点信息
     */
    std::string GetLocalEndpoint() const;
    
    /**
     * @brief 检查是否使用SSL
     */
    bool IsSSL() const { return ssl_stream_ != nullptr; }
    
    /**
     * @brief 获取会话ID
     */
    const std::string& GetSessionId() const { return session_id_; }

private:
    // HTTP请求处理
    void DoRead();
    void OnRead(boost::system::error_code ec, std::size_t bytes_transferred);
    void ProcessRequest();
    void SendResponse();
    void OnWrite(boost::system::error_code ec, std::size_t bytes_transferred, bool close);
    void DoClose();
    
    // SSL handshake handling
    void OnSSLHandshake(boost::system::error_code ec);
    
    // 错误处理
    void HandleError(boost::system::error_code ec, const std::string& operation);
    
    // Keep-Alive处理
    void HandleKeepAlive();
    
    // 工具函数
    std::string GenerateSessionId();
    void LogRequest();
    void UpdateStatistics();
    
    // 网络组件
    std::unique_ptr<boost::asio::ip::tcp::socket> socket_;
    std::unique_ptr<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> ssl_stream_;
    
    // HTTP处理组件
    boost::beast::flat_buffer buffer_;
    boost::beast::http::request<boost::beast::http::string_body> beast_request_;
    boost::beast::http::response<boost::beast::http::string_body> beast_response_;
    
    // 会话状态
    HttpServer& server_;
    std::string session_id_;
    std::chrono::steady_clock::time_point session_start_time_;
    std::atomic<bool> closed_{false};
    
    // 统计信息
    size_t requests_processed_ = 0;
    size_t bytes_received_ = 0;
    size_t bytes_sent_ = 0;
    
    // 当前请求上下文
    HttpRequest current_request_;
    HttpResponse current_response_;
    
    // Keep-Alive支持
    bool keep_alive_ = false;
    std::chrono::steady_clock::time_point last_activity_;
};

/**
 * @brief HTTP服务器配置
 */
struct HttpServerConfig {
    std::string bind_address = "0.0.0.0";      // 绑定地址
    uint16_t port = 8080;                      // 监听端口
    size_t max_connections = 1000;             // 最大连接数
    size_t thread_pool_size = std::thread::hardware_concurrency(); // 线程池大小
    std::chrono::seconds keep_alive_timeout{60}; // Keep-Alive超时
    std::chrono::seconds request_timeout{30};   // 请求超时
    size_t max_request_size = 10 * 1024 * 1024; // 最大请求大小 (10MB)
    size_t max_header_size = 64 * 1024;        // 最大头部大小 (64KB)
    bool enable_compression = true;             // 启用压缩
    std::string server_name = "Zeus-HTTP/1.0"; // 服务器名称
    
    // SSL配置
    bool enable_ssl = false;                   // 启用SSL
    std::string ssl_certificate_file;         // SSL证书文件
    std::string ssl_private_key_file;         // SSL私钥文件
    std::string ssl_dh_param_file;            // SSL DH参数文件
    bool ssl_verify_client = false;           // SSL客户端验证
    
    HttpServerConfig() = default;
};

/**
 * @brief HTTP服务器主类
 */
class HttpServer {
public:
    /**
     * @brief 构造函数
     * @param executor ASIO executor
     * @param config 服务器配置
     */
    explicit HttpServer(boost::asio::any_io_executor executor, const HttpServerConfig& config = HttpServerConfig{});
    
    /**
     * @brief 构造函数（使用线程池）
     * @param thread_count 线程数量
     * @param config 服务器配置
     */
    explicit HttpServer(size_t thread_count, const HttpServerConfig& config = HttpServerConfig{});
    
    /**
     * @brief 析构函数
     */
    ~HttpServer();
    
    // 服务器控制
    
    /**
     * @brief 启动服务器
     * @return 是否启动成功
     */
    bool Start();
    
    /**
     * @brief 停止服务器
     */
    void Stop();
    
    /**
     * @brief 检查服务器是否运行中
     */
    bool IsRunning() const { return running_; }
    
    /**
     * @brief 等待服务器停止
     */
    void Join();
    
    // 路由注册
    
    /**
     * @brief 注册GET路由
     */
    void Get(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册POST路由
     */
    void Post(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册PUT路由
     */
    void Put(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册DELETE路由
     */
    void Delete(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册PATCH路由
     */
    void Patch(const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册任意HTTP方法的路由
     */
    void Route(HttpMethod method, const std::string& path, HttpRequestHandler handler);
    
    /**
     * @brief 注册所有HTTP方法的路由
     */
    void All(const std::string& path, HttpRequestHandler handler);
    
    // 中间件支持
    
    /**
     * @brief 添加全局中间件
     */
    void Use(HttpRequestHandler middleware);
    
    /**
     * @brief 添加路径特定中间件
     */
    void Use(const std::string& path, HttpRequestHandler middleware);
    
    // 静态文件支持
    
    /**
     * @brief 启用静态文件服务
     * @param url_path URL路径前缀
     * @param file_path 本地文件系统路径
     */
    void ServeStatic(const std::string& url_path, const std::string& file_path);
    
    // 配置管理
    
    /**
     * @brief 获取服务器配置
     */
    const HttpServerConfig& GetConfig() const { return config_; }
    
    /**
     * @brief 更新服务器配置（仅在服务器停止时有效）
     */
    void SetConfig(const HttpServerConfig& config);
    
    // 统计信息
    
    /**
     * @brief 服务器统计信息
     */
    struct ServerStats {
        size_t active_connections = 0;
        size_t total_requests = 0;
        size_t successful_requests = 0;
        size_t failed_requests = 0;
        size_t total_bytes_received = 0;
        size_t total_bytes_sent = 0;
        std::chrono::steady_clock::time_point start_time;
        double average_response_time_ms = 0.0;
    };
    
    /**
     * @brief 获取服务器统计信息
     */
    ServerStats GetStats() const;
    
    /**
     * @brief 获取监听端点信息
     */
    std::string GetListeningEndpoint() const;
    
    // 内部使用方法（由HttpServerSession调用）
    bool ProcessRequest(const HttpRequest& request, HttpResponse& response);
    void UpdateStats(bool success, std::chrono::milliseconds response_time, size_t bytes_received, size_t bytes_sent);
    std::string GenerateServerHeader() const;
    // boost::asio::ssl::context* GetSSLContext() { return ssl_context_.get(); } // Temporarily disabled

private:
    // 服务器初始化
    void Initialize();
    void SetupSSL();
    
    // 连接处理
    void StartAccept();
    void HandleAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);
    // SSL accept handler
    void HandleSSLAccept(boost::system::error_code ec, boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream);
    
    // 路由处理
    RouteMatch MatchRoute(HttpMethod method, const std::string& path) const;
    void ExecuteMiddlewares(const HttpRequest& request, HttpResponse& response, 
                           const std::vector<HttpRequestHandler>& middlewares, size_t index);
    
    // 内建中间件
    void DefaultErrorHandler(const HttpRequest& request, HttpResponse& response, std::function<void()> next);
    void CompressionMiddleware(const HttpRequest& request, HttpResponse& response, std::function<void()> next);
    void LoggingMiddleware(const HttpRequest& request, HttpResponse& response, std::function<void()> next);
    
    // 静态文件处理
    bool HandleStaticFile(const HttpRequest& request, HttpResponse& response);
    std::string GetMimeType(const std::string& file_path);
    
    // Other utility functions
    
    boost::asio::any_io_executor executor_;
    HttpServerConfig config_;
    
    // 网络组件
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::unique_ptr<boost::asio::ssl::context> ssl_context_;
    
    // 路由系统
    struct RouteEntry {
        HttpMethod method;
        std::string path_pattern;
        std::regex path_regex;
        std::vector<std::string> param_names;
        HttpRequestHandler handler;
    };
    
    std::vector<RouteEntry> routes_;
    std::vector<HttpRequestHandler> global_middlewares_;
    std::unordered_map<std::string, std::vector<HttpRequestHandler>> path_middlewares_;
    
    // 静态文件映射
    std::unordered_map<std::string, std::string> static_paths_;
    
    // 服务器状态
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // 统计信息
    mutable std::mutex stats_mutex_;
    ServerStats stats_;
    
    // 会话管理
    std::vector<std::shared_ptr<HttpServerSession>> active_sessions_;
    std::mutex sessions_mutex_;
    
    // 线程管理（当使用内部线程池时）
    std::unique_ptr<boost::asio::io_context> owned_ioc_;
    std::vector<std::thread> worker_threads_;
};

/**
 * @brief HTTP服务器建造者模式
 */
class HttpServerBuilder {
public:
    HttpServerBuilder() = default;
    
    /**
     * @brief 设置绑定地址
     */
    HttpServerBuilder& BindTo(const std::string& address) {
        config_.bind_address = address;
        return *this;
    }
    
    /**
     * @brief 设置监听端口
     */
    HttpServerBuilder& ListenOn(uint16_t port) {
        config_.port = port;
        return *this;
    }
    
    /**
     * @brief 设置线程池大小
     */
    HttpServerBuilder& WithThreads(size_t thread_count) {
        thread_count_ = thread_count;
        return *this;
    }
    
    /**
     * @brief 设置最大连接数
     */
    HttpServerBuilder& WithMaxConnections(size_t max_connections) {
        config_.max_connections = max_connections;
        return *this;
    }
    
    /**
     * @brief 启用SSL
     */
    HttpServerBuilder& WithSSL(const std::string& cert_file, const std::string& key_file) {
        config_.enable_ssl = true;
        config_.ssl_certificate_file = cert_file;
        config_.ssl_private_key_file = key_file;
        return *this;
    }
    
    /**
     * @brief 设置服务器名称
     */
    HttpServerBuilder& WithServerName(const std::string& name) {
        config_.server_name = name;
        return *this;
    }
    
    /**
     * @brief 设置请求超时
     */
    HttpServerBuilder& WithRequestTimeout(std::chrono::seconds timeout) {
        config_.request_timeout = timeout;
        return *this;
    }
    
    /**
     * @brief 设置Keep-Alive超时
     */
    HttpServerBuilder& WithKeepAliveTimeout(std::chrono::seconds timeout) {
        config_.keep_alive_timeout = timeout;
        return *this;
    }
    
    /**
     * @brief 启用/禁用压缩
     */
    HttpServerBuilder& WithCompression(bool enable) {
        config_.enable_compression = enable;
        return *this;
    }
    
    /**
     * @brief 使用自定义executor
     */
    HttpServerBuilder& WithExecutor(boost::asio::any_io_executor executor) {
        executor_ = executor;
        return *this;
    }
    
    /**
     * @brief 构建HTTP服务器
     */
    std::unique_ptr<HttpServer> Build() {
        if (executor_) {
            return std::make_unique<HttpServer>(*executor_, config_);
        } else {
            return std::make_unique<HttpServer>(thread_count_, config_);
        }
    }

private:
    HttpServerConfig config_;
    size_t thread_count_ = std::thread::hardware_concurrency();
    std::optional<boost::asio::any_io_executor> executor_;
};

} // namespace http
} // namespace network
} // namespace common