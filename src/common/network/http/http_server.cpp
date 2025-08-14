#include "common/network/http/http_server.h"
#include "common/network/http/http_router.h"
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/bind_executor.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>

namespace common {
namespace network {
namespace http {

// ===== HttpServerSession Implementation =====

HttpServerSession::HttpServerSession(boost::asio::ip::tcp::socket socket, HttpServer& server)
    : server_(server), session_start_time_(std::chrono::steady_clock::now()) {
    socket_ = std::make_unique<boost::asio::ip::tcp::socket>(std::move(socket));
    session_id_ = GenerateSessionId();
    last_activity_ = std::chrono::steady_clock::now();
    
    NETWORK_LOG_DEBUG("HTTP session created: {}", session_id_);
}

HttpServerSession::HttpServerSession(boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream, HttpServer& server)
    : server_(server), session_start_time_(std::chrono::steady_clock::now()) {
    ssl_stream_ = std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(std::move(stream));
    session_id_ = GenerateSessionId();
    last_activity_ = std::chrono::steady_clock::now();
    
    NETWORK_LOG_DEBUG("HTTPS session created: {}", session_id_);
}

HttpServerSession::~HttpServerSession() {
    Close();
    NETWORK_LOG_DEBUG("HTTP session destroyed: {}", session_id_);
}

void HttpServerSession::Start() {
    if (ssl_stream_) {
        // SSL握手
        auto self = shared_from_this();
        ssl_stream_->async_handshake(
            boost::asio::ssl::stream_base::server,
            [self](boost::system::error_code ec) {
                self->OnSSLHandshake(ec);
            });
    } else {
        // 直接开始读取
        DoRead();
    }
}

void HttpServerSession::Close() {
    if (closed_.exchange(true)) {
        return;
    }
    
    // 关闭SSL流或常规socket
    if (ssl_stream_) {
        boost::system::error_code ec;
        ssl_stream_->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        ssl_stream_->lowest_layer().close(ec);
    } else if (socket_) {
        boost::system::error_code ec;
        socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_->close(ec);
    }
    
    NETWORK_LOG_DEBUG("HTTP session closed: {}", session_id_);
}

std::string HttpServerSession::GetRemoteEndpoint() const {
    try {
        if (ssl_stream_) {
            auto endpoint = ssl_stream_->lowest_layer().remote_endpoint();
            return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        } else if (socket_) {
            auto endpoint = socket_->remote_endpoint();
            return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        }
    } catch (const std::exception&) {
        // Socket可能已关闭
    }
    return "unknown";
}

std::string HttpServerSession::GetLocalEndpoint() const {
    try {
        if (ssl_stream_) {
            auto endpoint = ssl_stream_->lowest_layer().local_endpoint();
            return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        } else if (socket_) {
            auto endpoint = socket_->local_endpoint();
            return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
        }
    } catch (const std::exception&) {
        // Socket可能已关闭
    }
    return "unknown";
}

void HttpServerSession::OnSSLHandshake(boost::system::error_code ec) {
    if (ec) {
        NETWORK_LOG_ERROR("SSL handshake failed for session {}: {}", session_id_, ec.message());
        return;
    }
    
    NETWORK_LOG_DEBUG("SSL handshake completed for session: {}", session_id_);
    DoRead();
}

void HttpServerSession::DoRead() {
    if (closed_) {
        return;
    }
    
    // 设置超时
    beast_request_ = {};
    
    auto self = shared_from_this();
    
    if (ssl_stream_) {
        // SSL流读取
        boost::beast::http::async_read(
            *ssl_stream_, buffer_, beast_request_,
            [self](boost::system::error_code ec, std::size_t bytes_transferred) {
                self->OnRead(ec, bytes_transferred);
            });
    } else {
        // 普通Socket读取
        boost::beast::http::async_read(
            *socket_, buffer_, beast_request_,
            [self](boost::system::error_code ec, std::size_t bytes_transferred) {
                self->OnRead(ec, bytes_transferred);
            });
    }
}

void HttpServerSession::OnRead(boost::system::error_code ec, std::size_t bytes_transferred) {
    if (ec == boost::beast::http::error::end_of_stream) {
        // 客户端关闭连接
        DoClose();
        return;
    }
    
    if (ec) {
        HandleError(ec, "read");
        return;
    }
    
    bytes_received_ += bytes_transferred;
    last_activity_ = std::chrono::steady_clock::now();
    
    // 检查请求大小限制
    if (bytes_received_ > server_.GetConfig().max_request_size) {
        // 请求过大
        current_response_.SetStatusCode(HttpStatusCode::PAYLOAD_TOO_LARGE);
        current_response_.SetBody("Request too large");
        SendResponse();
        return;
    }
    
    ProcessRequest();
}

void HttpServerSession::ProcessRequest() {
    try {
        // 转换Beast请求到HttpRequest
        current_request_ = HttpRequest();
        
        // 设置方法
        auto method_str = std::string(beast_request_.method_string());
        if (method_str == "GET") current_request_.SetMethod(HttpMethod::GET);
        else if (method_str == "POST") current_request_.SetMethod(HttpMethod::POST);
        else if (method_str == "PUT") current_request_.SetMethod(HttpMethod::PUT);
        else if (method_str == "DELETE") current_request_.SetMethod(HttpMethod::DELETE);
        else if (method_str == "PATCH") current_request_.SetMethod(HttpMethod::PATCH);
        else if (method_str == "HEAD") current_request_.SetMethod(HttpMethod::HEAD);
        else if (method_str == "OPTIONS") current_request_.SetMethod(HttpMethod::OPTIONS);
        else current_request_.SetMethod(HttpMethod::CUSTOM);
        
        // 设置URL
        std::string target = std::string(beast_request_.target());
        current_request_.SetUrl(target);
        
        // 设置头部
        for (const auto& field : beast_request_) {
            std::string name(field.name_string());
            std::string value(field.value());
            current_request_.SetHeader(name, value);
        }
        
        // 设置请求体
        current_request_.SetBody(beast_request_.body());
        
        // 检查Keep-Alive
        keep_alive_ = beast_request_.keep_alive();
        
        // 初始化响应
        current_response_ = HttpResponse();
        current_response_.SetStatusCode(HttpStatusCode::OK);
        current_response_.SetHeader("Server", server_.GenerateServerHeader());
        
        // 记录请求日志
        LogRequest();
        
        // 处理请求
        auto start_time = std::chrono::steady_clock::now();
        bool handled = server_.ProcessRequest(current_request_, current_response_);
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        if (!handled) {
            current_response_.SetStatusCode(HttpStatusCode::NOT_FOUND);
            current_response_.SetBody("Not Found");
        }
        
        // 更新统计
        server_.UpdateStats(handled, duration, bytes_received_, current_response_.GetBody().size());
        requests_processed_++;
        
        SendResponse();
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Error processing request in session {}: {}", session_id_, e.what());
        current_response_.SetStatusCode(HttpStatusCode::INTERNAL_SERVER_ERROR);
        current_response_.SetBody("Internal Server Error");
        SendResponse();
    }
}

void HttpServerSession::SendResponse() {
    // 设置连接头
    if (keep_alive_) {
        current_response_.SetHeader("Connection", "keep-alive");
        current_response_.SetHeader("Keep-Alive", "timeout=" + std::to_string(server_.GetConfig().keep_alive_timeout.count()));
    } else {
        current_response_.SetHeader("Connection", "close");
    }
    
    // 转换HttpResponse到Beast响应
    beast_response_ = {};
    beast_response_.version(beast_request_.version());
    beast_response_.result(static_cast<boost::beast::http::status>(static_cast<int>(current_response_.GetStatusCode())));
    beast_response_.keep_alive(keep_alive_);
    
    // 设置头部
    for (const auto& header : current_response_.GetHeaders()) {
        beast_response_.set(header.first, header.second);
    }
    
    // 设置响应体
    beast_response_.body() = current_response_.GetBody();
    beast_response_.prepare_payload();
    
    auto self = shared_from_this();
    
    if (ssl_stream_) {
        // SSL流写入
        boost::beast::http::async_write(
            *ssl_stream_, beast_response_,
            [self](boost::system::error_code ec, std::size_t bytes_transferred) {
                self->OnWrite(ec, bytes_transferred, !self->keep_alive_);
            });
    } else {
        // 普通Socket写入
        boost::beast::http::async_write(
            *socket_, beast_response_,
            [self](boost::system::error_code ec, std::size_t bytes_transferred) {
                self->OnWrite(ec, bytes_transferred, !self->keep_alive_);
            });
    }
}

void HttpServerSession::OnWrite(boost::system::error_code ec, std::size_t bytes_transferred, bool close) {
    if (ec) {
        HandleError(ec, "write");
        return;
    }
    
    bytes_sent_ += bytes_transferred;
    last_activity_ = std::chrono::steady_clock::now();
    
    if (close) {
        DoClose();
        return;
    }
    
    // 继续处理下一个请求
    DoRead();
}

void HttpServerSession::DoClose() {
    Close();
}

void HttpServerSession::HandleError(boost::system::error_code ec, const std::string& operation) {
    if (ec == boost::asio::error::operation_aborted) {
        return;
    }
    
    NETWORK_LOG_DEBUG("HTTP session {} {} error: {}", session_id_, operation, ec.message());
    DoClose();
}

std::string HttpServerSession::GenerateSessionId() {
    std::ostringstream oss;
    oss << "http_" << std::this_thread::get_id() << "_" 
        << std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
    return oss.str();
}

void HttpServerSession::LogRequest() {
    NETWORK_LOG_INFO("HTTP {} {} {} - Session: {}", 
                     static_cast<int>(current_request_.GetMethod()),
                     current_request_.GetUrl().path,
                     GetRemoteEndpoint(),
                     session_id_);
}

// ===== HttpServer Implementation =====

HttpServer::HttpServer(boost::asio::any_io_executor executor, const HttpServerConfig& config)
    : executor_(executor), config_(config) {
    Initialize();
}

HttpServer::HttpServer(size_t thread_count, const HttpServerConfig& config)
    : config_(config) {
    owned_ioc_ = std::make_unique<boost::asio::io_context>();
    executor_ = owned_ioc_->get_executor();
    
    // 创建工作线程
    worker_threads_.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back([this]() {
            owned_ioc_->run();
        });
    }
    
    Initialize();
}

HttpServer::~HttpServer() {
    Stop();
    Join();
}

void HttpServer::Initialize() {
    // 初始化acceptor
    acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(executor_);
    
    // 初始化SSL（如果启用）
    if (config_.enable_ssl) {
        SetupSSL();
    }
    
    // 添加默认中间件
    global_middlewares_.push_back([this](const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
        LoggingMiddleware(request, response, next);
    });
    
    global_middlewares_.push_back([this](const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
        DefaultErrorHandler(request, response, next);
    });
    
    stats_.start_time = std::chrono::steady_clock::now();
    
    NETWORK_LOG_INFO("HTTP server initialized on {}:{}", config_.bind_address, config_.port);
}

void HttpServer::SetupSSL() {
    try {
        // 创建SSL上下文
        ssl_context_ = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_server);
        
        // 配置SSL选项
        ssl_context_->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::single_dh_use);
        
        // 加载证书和私钥
        if (!config_.ssl_certificate_file.empty()) {
            ssl_context_->use_certificate_chain_file(config_.ssl_certificate_file);
        }
        
        if (!config_.ssl_private_key_file.empty()) {
            ssl_context_->use_private_key_file(config_.ssl_private_key_file, boost::asio::ssl::context::pem);
        }
        
        // 加载DH参数（如果提供）
        if (!config_.ssl_dh_param_file.empty()) {
            ssl_context_->use_tmp_dh_file(config_.ssl_dh_param_file);
        }
        
        // 设置验证模式
        if (config_.ssl_verify_client) {
            ssl_context_->set_verify_mode(boost::asio::ssl::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
        } else {
            ssl_context_->set_verify_mode(boost::asio::ssl::verify_none);
        }
        
        NETWORK_LOG_INFO("SSL context initialized successfully");
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to setup SSL: {}", e.what());
        ssl_context_.reset();
        throw;
    }
}

bool HttpServer::Start() {
    if (running_.exchange(true)) {
        NETWORK_LOG_WARN("HTTP server already running");
        return false;
    }
    
    try {
        boost::asio::ip::tcp::endpoint endpoint(
            boost::asio::ip::make_address(config_.bind_address), 
            config_.port);
        
        acceptor_->open(endpoint.protocol());
        acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_->bind(endpoint);
        acceptor_->listen(boost::asio::socket_base::max_listen_connections);
        
        NETWORK_LOG_INFO("HTTP server started on {}", GetListeningEndpoint());
        
        StartAccept();
        return true;
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to start HTTP server: {}", e.what());
        running_ = false;
        return false;
    }
}

void HttpServer::Stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    shutdown_requested_ = true;
    
    boost::system::error_code ec;
    if (acceptor_) {
        acceptor_->close(ec);
    }
    
    // 关闭所有活动会话
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        for (auto& session : active_sessions_) {
            session->Close();
        }
        active_sessions_.clear();
    }
    
    if (owned_ioc_) {
        owned_ioc_->stop();
    }
    
    NETWORK_LOG_INFO("HTTP server stopped");
}

void HttpServer::Join() {
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();
}

// IsRunning() is defined inline in header file

void HttpServer::StartAccept() {
    if (!running_) {
        return;
    }
    
    if (ssl_context_) {
        // HTTPS - 创建SSL流
        auto ssl_socket = std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(executor_, *ssl_context_);
        auto& lowest_layer = ssl_socket->lowest_layer();
        
        acceptor_->async_accept(lowest_layer,
            [this, ssl_stream = std::move(ssl_socket)](boost::system::error_code ec) mutable {
                if (!ec) {
                    HandleSSLAccept(ec, std::move(*ssl_stream));
                } else if (running_) {
                    NETWORK_LOG_ERROR("HTTPS accept error: {}", ec.message());
                }
                StartAccept();
            });
    } else {
        // HTTP - 普通socket
        acceptor_->async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                HandleAccept(ec, std::move(socket));
                StartAccept();
            });
    }
}

void HttpServer::HandleAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (ec) {
        if (running_) {
            NETWORK_LOG_ERROR("HTTP accept error: {}", ec.message());
        }
        return;
    }
    
    // 检查连接限制
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        if (active_sessions_.size() >= config_.max_connections) {
            NETWORK_LOG_WARN("HTTP server rejecting connection - maximum connections reached ({})", config_.max_connections);
            boost::system::error_code close_ec;
            socket.close(close_ec);
            return;
        }
        
        auto session = std::make_shared<HttpServerSession>(std::move(socket), *this);
        active_sessions_.push_back(session);
        
        // 启动会话
        session->Start();
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.active_connections++;
    }
}

void HttpServer::HandleSSLAccept(boost::system::error_code ec, boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream) {
    if (ec) {
        if (running_) {
            NETWORK_LOG_ERROR("HTTPS accept error: {}", ec.message());
        }
        return;
    }
    
    // 检查连接限制
    {
        std::lock_guard<std::mutex> lock(sessions_mutex_);
        if (active_sessions_.size() >= config_.max_connections) {
            NETWORK_LOG_WARN("HTTPS server rejecting connection - maximum connections reached ({})", config_.max_connections);
            boost::system::error_code close_ec;
            stream.lowest_layer().close(close_ec);
            return;
        }
        
        auto session = std::make_shared<HttpServerSession>(std::move(stream), *this);
        active_sessions_.push_back(session);
        
        // 启动会话
        session->Start();
    }
    
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.active_connections++;
    }
}

// 路由注册方法
void HttpServer::Get(const std::string& path, HttpRequestHandler handler) {
    Route(HttpMethod::GET, path, std::move(handler));
}

void HttpServer::Post(const std::string& path, HttpRequestHandler handler) {
    Route(HttpMethod::POST, path, std::move(handler));
}

void HttpServer::Put(const std::string& path, HttpRequestHandler handler) {
    Route(HttpMethod::PUT, path, std::move(handler));
}

void HttpServer::Delete(const std::string& path, HttpRequestHandler handler) {
    Route(HttpMethod::DELETE, path, std::move(handler));
}

void HttpServer::Patch(const std::string& path, HttpRequestHandler handler) {
    Route(HttpMethod::PATCH, path, std::move(handler));
}

void HttpServer::Route(HttpMethod method, const std::string& path, HttpRequestHandler handler) {
    RouteEntry entry;
    entry.method = method;
    entry.path_pattern = path;
    entry.handler = std::move(handler);
    
    // 编译路径正则表达式（简化版本）
    std::string regex_pattern = path;
    // 将{param}替换为捕获组
    std::regex param_regex(R"(\{([^}]+)\})");
    std::sregex_iterator iter(regex_pattern.begin(), regex_pattern.end(), param_regex);
    std::sregex_iterator end;
    
    size_t offset = 0;
    for (; iter != end; ++iter) {
        const std::smatch& match = *iter;
        entry.param_names.push_back(match[1].str());
        
        size_t pos = match.position() + offset;
        size_t len = match.length();
        regex_pattern.replace(pos, len, "([^/]+)");
        offset += 7 - len; // "([^/]+)" has 7 chars, original match has len chars
    }
    
    entry.path_regex = std::regex(regex_pattern);
    routes_.push_back(std::move(entry));
}

void HttpServer::All(const std::string& path, HttpRequestHandler handler) {
    // 为所有常见HTTP方法注册相同的处理器
    std::vector<HttpMethod> methods = {
        HttpMethod::GET, HttpMethod::POST, HttpMethod::PUT, 
        HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD, HttpMethod::OPTIONS
    };
    
    for (auto method : methods) {
        Route(method, path, handler);
    }
}

void HttpServer::Use(HttpRequestHandler middleware) {
    global_middlewares_.push_back(std::move(middleware));
}

void HttpServer::Use(const std::string& path, HttpRequestHandler middleware) {
    path_middlewares_[path].push_back(std::move(middleware));
}

void HttpServer::ServeStatic(const std::string& url_path, const std::string& file_path) {
    static_paths_[url_path] = file_path;
    
    // 注册通配符路由处理静态文件
    std::string pattern = url_path;
    if (pattern.back() != '/') pattern += '/';
    pattern += "*";
    
    Get(pattern, [this](const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
        if (!HandleStaticFile(request, response)) {
            next();
        }
    });
}

bool HttpServer::ProcessRequest(const HttpRequest& request, HttpResponse& response) {
    try {
        // 首先检查静态文件
        if (HandleStaticFile(request, response)) {
            return true;
        }
        
        // 查找匹配的路由
        RouteMatch match = MatchRoute(request.GetMethod(), request.GetUrl().path);
        if (!match.matched) {
            return false;
        }
        
        // 准备中间件列表
        std::vector<HttpRequestHandler> middlewares = global_middlewares_;
        
        // 添加路径特定中间件
        for (const auto& path_middleware : path_middlewares_) {
            if (request.GetUrl().path.find(path_middleware.first) == 0) {
                middlewares.insert(middlewares.end(), 
                                 path_middleware.second.begin(), 
                                 path_middleware.second.end());
            }
        }
        
        // 执行中间件链
        ExecuteMiddlewares(request, response, middlewares, 0);
        
        return true;
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Error processing HTTP request: {}", e.what());
        response.SetStatusCode(HttpStatusCode::INTERNAL_SERVER_ERROR);
        response.SetBody("Internal Server Error");
        return true;
    }
}

RouteMatch HttpServer::MatchRoute(HttpMethod method, const std::string& path) const {
    RouteMatch result;
    
    for (const auto& route : routes_) {
        if (route.method != method) {
            continue;
        }
        
        std::smatch match;
        if (std::regex_match(path, match, route.path_regex)) {
            result.matched = true;
            result.matched_path = path;
            
            // 提取参数
            for (size_t i = 1; i < match.size() && (i - 1) < route.param_names.size(); ++i) {
                result.params[route.param_names[i - 1]] = match[i].str();
            }
            
            break;
        }
    }
    
    return result;
}

void HttpServer::ExecuteMiddlewares(const HttpRequest& request, HttpResponse& response, 
                                   const std::vector<HttpRequestHandler>& middlewares, size_t index) {
    if (index >= middlewares.size()) {
        return;
    }
    
    auto next = [this, &request, &response, &middlewares, index]() {
        ExecuteMiddlewares(request, response, middlewares, index + 1);
    };
    
    middlewares[index](request, response, next);
}

void HttpServer::DefaultErrorHandler(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    try {
        next();
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Unhandled exception in request handler: {}", e.what());
        response.SetStatusCode(HttpStatusCode::INTERNAL_SERVER_ERROR);
        response.SetBody("Internal Server Error");
    }
}

void HttpServer::LoggingMiddleware(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    auto start_time = std::chrono::steady_clock::now();
    
    next();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    NETWORK_LOG_INFO("HTTP {} {} {} - {}ms", 
                     static_cast<int>(request.GetMethod()),
                     request.GetUrl().path,
                     static_cast<int>(response.GetStatusCode()),
                     duration.count());
}

bool HttpServer::HandleStaticFile(const HttpRequest& request, HttpResponse& response) {
    std::string request_path = request.GetUrl().path;
    
    for (const auto& static_mapping : static_paths_) {
        const std::string& url_prefix = static_mapping.first;
        const std::string& file_root = static_mapping.second;
        
        if (request_path.find(url_prefix) == 0) {
            std::string relative_path = request_path.substr(url_prefix.length());
            std::string file_path = file_root + "/" + relative_path;
            
            std::ifstream file(file_path, std::ios::binary);
            if (file) {
                std::ostringstream buffer;
                buffer << file.rdbuf();
                
                response.SetStatusCode(HttpStatusCode::OK);
                response.SetBody(buffer.str());
                response.SetHeader("Content-Type", GetMimeType(file_path));
                return true;
            }
        }
    }
    
    return false;
}

std::string HttpServer::GetMimeType(const std::string& file_path) {
    auto pos = file_path.find_last_of('.');
    if (pos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = file_path.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    static std::unordered_map<std::string, std::string> mime_types = {
        {"html", "text/html"},
        {"htm", "text/html"},
        {"css", "text/css"},
        {"js", "application/javascript"},
        {"json", "application/json"},
        {"xml", "application/xml"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"gif", "image/gif"},
        {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"},
        {"txt", "text/plain"},
        {"pdf", "application/pdf"},
        {"zip", "application/zip"}
    };
    
    auto it = mime_types.find(ext);
    return (it != mime_types.end()) ? it->second : "application/octet-stream";
}

std::string HttpServer::GenerateServerHeader() const {
    return config_.server_name;
}

HttpServer::ServerStats HttpServer::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

std::string HttpServer::GetListeningEndpoint() const {
    if (acceptor_ && acceptor_->is_open()) {
        auto endpoint = acceptor_->local_endpoint();
        return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
    }
    return "not_listening";
}

void HttpServer::UpdateStats(bool success, std::chrono::milliseconds response_time, size_t bytes_received, size_t bytes_sent) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_requests++;
    if (success) {
        stats_.successful_requests++;
    } else {
        stats_.failed_requests++;
    }
    
    stats_.total_bytes_received += bytes_received;
    stats_.total_bytes_sent += bytes_sent;
    
    // 更新平均响应时间
    if (stats_.total_requests > 0) {
        stats_.average_response_time_ms = 
            (stats_.average_response_time_ms * (stats_.total_requests - 1) + response_time.count()) / stats_.total_requests;
    }
}

void HttpServer::SetConfig(const HttpServerConfig& config) {
    if (running_) {
        NETWORK_LOG_WARN("Cannot update HTTP server config while running");
        return;
    }
    config_ = config;
}

} // namespace http
} // namespace network
} // namespace common