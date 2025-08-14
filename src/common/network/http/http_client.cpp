#include "common/network/http/http_client.h"
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <fstream>
#include <random>

namespace common {
namespace network {
namespace http {

// HttpSession Implementation
HttpSession::HttpSession(boost::asio::any_io_executor executor, const HttpConfig& config)
    : executor_(executor), config_(config), resolver_(executor) {
    
    // Generate unique session ID
    static std::atomic<uint64_t> session_counter{1};
    session_id_ = "http_session_" + std::to_string(session_counter.fetch_add(1));
    
    NETWORK_LOG_DEBUG("HTTP session created: {}", session_id_);
}

HttpSession::~HttpSession() {
    Cancel();
    Disconnect();
    NETWORK_LOG_DEBUG("HTTP session destroyed: {}", session_id_);
}

void HttpSession::AsyncRequest(const HttpRequest& request, 
                              HttpResponseCallback callback,
                              HttpProgressCallback progress_callback) {
    if (busy_.exchange(true)) {
        if (callback) {
            boost::asio::post(executor_, [callback]() {
                callback(boost::asio::error::operation_aborted, HttpResponse{});
            });
        }
        return;
    }
    
    cancelled_ = false;
    request_start_time_ = std::chrono::steady_clock::now();
    bytes_sent_ = 0;
    bytes_received_ = 0;
    
    NETWORK_LOG_DEBUG("Starting HTTP request: {} {}", 
                     HttpUtils::MethodToString(request.GetMethod()), 
                     request.GetUrl().ToString());
    
    DoRequest(request, callback, progress_callback);
}

HttpResponse HttpSession::Request(const HttpRequest& request, std::chrono::milliseconds timeout_ms) {
    std::promise<HttpResponse> promise;
    std::future<HttpResponse> future = promise.get_future();
    
    HttpException last_error{HttpErrorCode::UNKNOWN_ERROR, "Request failed"};
    
    AsyncRequest(request, [&promise, &last_error](boost::system::error_code ec, const HttpResponse& response) {
        if (ec) {
            last_error = HttpException{HttpErrorCode::UNKNOWN_ERROR, ec.message()};
            promise.set_value(HttpResponse{HttpStatusCode::INTERNAL_SERVER_ERROR});
        } else {
            promise.set_value(response);
        }
    });
    
    if (future.wait_for(timeout_ms) == std::future_status::timeout) {
        Cancel();
        throw HttpException{HttpErrorCode::REQUEST_TIMEOUT, "Request timeout"};
    }
    
    HttpResponse response = future.get();
    if (response.GetStatusCode() == HttpStatusCode::INTERNAL_SERVER_ERROR && 
        response.GetBody().empty()) {
        throw last_error;
    }
    
    return response;
}

void HttpSession::Cancel() {
    cancelled_ = true;
    CancelTimeout();
    
    if (ssl_stream_) {
        boost::system::error_code ec;
        ssl_stream_->lowest_layer().cancel(ec);
    } else if (socket_ && socket_->is_open()) {
        boost::system::error_code ec;
        socket_->cancel(ec);
    }
    
    busy_ = false;
}

void HttpSession::Connect(const HttpUrl& url, std::function<void(boost::system::error_code)> callback) {
    current_url_ = url;
    
    // Check if we need SSL
    bool use_ssl = url.IsSecure();
    
    if (use_ssl && !ssl_context_) {
        SetupSSL(url);
    }
    
    // Resolve hostname
    auto self = shared_from_this();
    std::string port = std::to_string(url.port == 0 ? url.GetDefaultPort() : url.port);
    
    NETWORK_LOG_DEBUG("Resolving {}:{}", url.host, port);
    
    resolver_.async_resolve(url.host, port,
        [self, callback](boost::system::error_code ec, 
                         boost::asio::ip::tcp::resolver::results_type results) {
            if (ec) {
                NETWORK_LOG_ERROR("DNS resolution failed for {}: {}", self->current_url_.host, ec.message());
                self->HandleError(ec, "DNS resolution");
                if (callback) callback(ec);
                return;
            }
            
            // Check if we need SSL from the URL
            bool use_ssl = self->current_url_.IsSecure();
            
            if (use_ssl) {
                // Create SSL stream if needed
                if (!self->ssl_stream_ && self->ssl_context_) {
                    auto socket = std::make_unique<boost::asio::ip::tcp::socket>(self->executor_);
                    self->ssl_stream_ = std::make_unique<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>(
                        std::move(*socket), *self->ssl_context_);
                }
                
                // Connect SSL stream
                if (self->ssl_stream_) {
                    boost::asio::async_connect(self->ssl_stream_->lowest_layer(), results,
                        [self, callback](boost::system::error_code ec, const boost::asio::ip::tcp::endpoint&) {
                            if (ec) {
                                NETWORK_LOG_ERROR("TCP connect failed: {}", ec.message());
                                self->HandleError(ec, "TCP connection");
                                if (callback) callback(ec);
                                return;
                            }
                            
                            NETWORK_LOG_DEBUG("TCP connected successfully, starting SSL handshake");
                            self->PerformSSLHandshake(callback);
                        });
                }
            } else {
                // HTTP connection - create regular socket if needed
                if (!self->socket_) {
                    self->socket_ = std::make_unique<boost::asio::ip::tcp::socket>(self->executor_);
                }
                
                boost::asio::async_connect(*self->socket_, results,
                    [self, callback](boost::system::error_code ec, const boost::asio::ip::tcp::endpoint&) {
                        if (ec) {
                            NETWORK_LOG_ERROR("TCP connect failed: {}", ec.message());
                            self->HandleError(ec, "TCP connection");
                            if (callback) callback(ec);
                            return;
                        }
                        
                        NETWORK_LOG_DEBUG("TCP connected successfully");
                        if (callback) callback(boost::system::error_code{});
                    });
            }
        });
}

void HttpSession::Disconnect() {
    // Close SSL stream
    if (ssl_stream_) {
        boost::system::error_code ec;
        ssl_stream_->lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        ssl_stream_->lowest_layer().close(ec);
        ssl_stream_.reset();
    }
    
    // Close regular HTTP socket
    if (socket_) {
        boost::system::error_code ec;
        socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_->close(ec);
        socket_.reset();
    }
    
    current_url_ = HttpUrl{};
}

bool HttpSession::IsConnected() const {
    // Check SSL stream connection
    if (ssl_stream_) {
        return ssl_stream_->lowest_layer().is_open();
    }
    
    // Check regular HTTP socket connection
    if (socket_) {
        return socket_->is_open();
    }
    
    return false;
}

void HttpSession::DoRequest(const HttpRequest& request, HttpResponseCallback callback, HttpProgressCallback progress_callback) {
    if (cancelled_) {
        if (callback) {
            callback(boost::asio::error::operation_aborted, HttpResponse{});
        }
        busy_ = false;
        return;
    }
    
    // Check if we need to connect or reconnect
    if (!IsConnected() || current_url_.host != request.GetUrl().host || 
        current_url_.port != request.GetUrl().port || current_url_.scheme != request.GetUrl().scheme) {
        
        Disconnect();
        
        auto self = shared_from_this();
        Connect(request.GetUrl(), [self, request, callback, progress_callback](boost::system::error_code ec) {
            if (ec) {
                if (callback) {
                    callback(ec, HttpResponse{});
                }
                self->busy_ = false;
                return;
            }
            
            self->DoRequest(request, callback, progress_callback);
        });
        return;
    }
    
    // Start timeout
    StartTimeout(config_.request_timeout);
    
    // Convert to Beast request
    beast_request_ = request.ToBeastRequest();
    
    // Add required headers
    beast_request_.set(boost::beast::http::field::host, current_url_.host);
    if (!beast_request_.has_content_length() && !beast_request_.body().empty()) {
        beast_request_.prepare_payload();
    }
    
    WriteRequest(request);
}

void HttpSession::WriteRequest(const HttpRequest& request) {
    auto self = shared_from_this();
    
    // Fire send event
    FireNetworkEvent(NetworkEventType::DATA_SENT);
    
    if (ssl_stream_) {
        // SSL写入
        boost::beast::http::async_write(*ssl_stream_, beast_request_,
            [self, request](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    NETWORK_LOG_ERROR("HTTPS write failed: {}", ec.message());
                    self->HandleError(ec, "HTTPS write");
                    return;
                }
                
                self->bytes_sent_ = bytes_transferred;
                NETWORK_LOG_TRACE("HTTPS request sent: {} bytes", bytes_transferred);
                
                // Start reading response
                self->ReadResponse([self](boost::system::error_code ec, const HttpResponse& response) {
                    // This callback will be handled by ReadResponse
                });
            });
    } else {
        // HTTP写入
        boost::beast::http::async_write(*socket_, beast_request_,
            [self, request](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    NETWORK_LOG_ERROR("HTTP write failed: {}", ec.message());
                    self->HandleError(ec, "HTTP write");
                    return;
                }
                
                self->bytes_sent_ = bytes_transferred;
                NETWORK_LOG_TRACE("HTTP request sent: {} bytes", bytes_transferred);
                
                // Start reading response
                self->ReadResponse([self](boost::system::error_code ec, const HttpResponse& response) {
                    // This callback will be handled by ReadResponse
                });
            });
    }
}

void HttpSession::ReadResponse(HttpResponseCallback callback) {
    auto self = shared_from_this();
    
    if (ssl_stream_) {
        // SSL读取
        boost::beast::http::async_read(*ssl_stream_, buffer_, beast_response_,
            [self, callback](boost::system::error_code ec, std::size_t bytes_transferred) {
                self->CancelTimeout();
                
                if (ec) {
                    NETWORK_LOG_ERROR("HTTPS read failed: {}", ec.message());
                    self->HandleError(ec, "HTTPS read");
                    if (callback) {
                        callback(ec, HttpResponse{});
                    }
                    return;
                }
                
                self->bytes_received_ = bytes_transferred;
                NETWORK_LOG_TRACE("HTTPS response received: {} bytes", bytes_transferred);
                
                // Convert response
                HttpResponse response = HttpResponse::FromBeastResponse(self->beast_response_);
                
                // Fire receive event
                self->FireNetworkEvent(NetworkEventType::DATA_RECEIVED);
                
                self->busy_ = false;
                
                if (callback) {
                    callback(boost::system::error_code{}, response);
                }
            });
    } else {
        // HTTP读取
        boost::beast::http::async_read(*socket_, buffer_, beast_response_,
            [self, callback](boost::system::error_code ec, std::size_t bytes_transferred) {
                self->CancelTimeout();
                
                if (ec) {
                    NETWORK_LOG_ERROR("HTTP read failed: {}", ec.message());
                    self->HandleError(ec, "HTTP read");
                    if (callback) {
                        callback(ec, HttpResponse{});
                    }
                    return;
                }
                
                self->bytes_received_ = bytes_transferred;
                NETWORK_LOG_TRACE("HTTP response received: {} bytes", bytes_transferred);
                
                // Convert response
                HttpResponse response = HttpResponse::FromBeastResponse(self->beast_response_);
                
                // Fire receive event
                self->FireNetworkEvent(NetworkEventType::DATA_RECEIVED);
                
                self->busy_ = false;
                
                if (callback) {
                    callback(boost::system::error_code{}, response);
                }
            });
    }
}

void HttpSession::HandleRedirect(const HttpResponse& response, const HttpRequest& original_request, 
                                HttpResponseCallback callback, HttpProgressCallback progress_callback, size_t redirect_count) {
    // Handle redirect logic (simplified implementation)
    if (redirect_count >= config_.max_redirects) {
        if (callback) {
            callback(boost::asio::error::invalid_argument, response);
        }
        busy_ = false;
        return;
    }
    
    std::string location = response.GetHeader("Location");
    if (location.empty()) {
        if (callback) {
            callback(boost::system::error_code{}, response);
        }
        busy_ = false;
        return;
    }
    
    // Create new request for redirect
    HttpRequest redirect_request = original_request;
    redirect_request.SetUrl(location);
    
    // For POST/PUT/PATCH, change to GET on certain status codes
    auto status = static_cast<int>(response.GetStatusCode());
    if ((status == 301 || status == 302 || status == 303) && 
        (original_request.GetMethod() == HttpMethod::POST || 
         original_request.GetMethod() == HttpMethod::PUT ||
         original_request.GetMethod() == HttpMethod::PATCH)) {
        redirect_request.SetMethod(HttpMethod::GET);
        redirect_request.SetBody(""); // Clear body for GET
    }
    
    NETWORK_LOG_DEBUG("Following redirect to: {}", location);
    
    DoRequest(redirect_request, callback, progress_callback);
}

void HttpSession::SetupSSL(const HttpUrl& url) {
    try {
        // 创建SSL上下文
        ssl_context_ = std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
        
        // 配置SSL选项
        ssl_context_->set_default_verify_paths();
        ssl_context_->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::single_dh_use);
        
        // 设置验证模式
        if (config_.verify_ssl) {
            ssl_context_->set_verify_mode(boost::asio::ssl::verify_peer);
            ssl_context_->set_verify_callback(boost::asio::ssl::host_name_verification(url.host));
        } else {
            ssl_context_->set_verify_mode(boost::asio::ssl::verify_none);
        }
        
        NETWORK_LOG_DEBUG("SSL context initialized for {}", url.host);
        
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to setup SSL for {}: {}", url.host, e.what());
        ssl_context_.reset();
        throw;
    }
}

void HttpSession::PerformSSLHandshake(std::function<void(boost::system::error_code)> callback) {
    if (!ssl_stream_) {
        if (callback) {
            callback(boost::asio::error::not_connected);
        }
        return;
    }
    
    NETWORK_LOG_DEBUG("Starting SSL handshake with {}", current_url_.host);
    
    // 设置SNI (Server Name Indication)
    if (!SSL_set_tlsext_host_name(ssl_stream_->native_handle(), current_url_.host.c_str())) {
        NETWORK_LOG_WARN("Failed to set SNI for {}", current_url_.host);
    }
    
    auto self = shared_from_this();
    ssl_stream_->async_handshake(
        boost::asio::ssl::stream_base::client,
        [self, callback](boost::system::error_code ec) {
            if (ec) {
                NETWORK_LOG_ERROR("SSL handshake failed with {}: {}", self->current_url_.host, ec.message());
                if (callback) callback(ec);
                return;
            }
            
            NETWORK_LOG_DEBUG("SSL handshake completed with {}", self->current_url_.host);
            if (callback) callback(boost::system::error_code{});
        });
}

void HttpSession::StartTimeout(std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0) {
        return;
    }
    
    CancelTimeout();
    
    timeout_timer_ = std::make_unique<boost::asio::steady_timer>(executor_);
    timeout_timer_->expires_after(timeout);
    
    auto self = shared_from_this();
    timeout_timer_->async_wait([self](boost::system::error_code ec) {
        if (!ec && !self->cancelled_) {
            NETWORK_LOG_WARN("HTTP request timeout: {}", self->session_id_);
            self->Cancel();
        }
    });
}

void HttpSession::CancelTimeout() {
    if (timeout_timer_) {
        boost::system::error_code ec;
        timeout_timer_->cancel();
        timeout_timer_.reset();
    }
}

void HttpSession::HandleError(boost::system::error_code ec, const std::string& operation) {
    NETWORK_LOG_ERROR("HTTP session error in {}: {}", operation, ec.message());
    
    FireNetworkEvent(NetworkEventType::CONNECTION_ERROR, operation + ": " + ec.message());
    
    busy_ = false;
}

void HttpSession::FireNetworkEvent(NetworkEventType type, const std::string& details) {
    try {
        NetworkEvent event(type);
        event.connection_id = session_id_;
        event.endpoint = current_url_.host + ":" + std::to_string(current_url_.port);
        event.protocol = "HTTP";
        event.bytes_transferred = (type == NetworkEventType::DATA_SENT) ? bytes_sent_ : bytes_received_;
        
        if (!details.empty()) {
            event.error_message = details;
        }
        
        NetworkEventManager::Instance().FireEvent(event);
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to fire network event: {}", e.what());
    }
}

// HttpClient Implementation
HttpClient::HttpClient(boost::asio::any_io_executor executor, const HttpConfig& config)
    : executor_(executor), config_(config) {
    
    // Create initial session pool
    for (size_t i = 0; i < std::min(max_pool_size_, size_t{2}); ++i) {
        CreateSession();
    }
    
    NETWORK_LOG_DEBUG("HTTP client created with executor");
}

HttpClient::HttpClient(size_t thread_count, const HttpConfig& config)
    : config_(config) {
    
    // Create our own io_context and thread pool
    owned_ioc_ = std::make_unique<boost::asio::io_context>();
    executor_ = owned_ioc_->get_executor();
    
    // Start worker threads
    worker_threads_.reserve(thread_count);
    for (size_t i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back([this]() {
            owned_ioc_->run();
        });
    }
    
    // Create initial session pool
    for (size_t i = 0; i < std::min(max_pool_size_, size_t{2}); ++i) {
        CreateSession();
    }
    
    NETWORK_LOG_DEBUG("HTTP client created with {} threads", thread_count);
}

HttpClient::~HttpClient() {
    CancelAllRequests();
    
    if (owned_ioc_) {
        owned_ioc_->stop();
        for (auto& thread : worker_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    
    NETWORK_LOG_DEBUG("HTTP client destroyed");
}

// Async methods implementation
void HttpClient::Get(const std::string& url, HttpResponseCallback callback, 
                    const HttpHeaders& headers, HttpProgressCallback progress_callback) {
    HttpRequest request(HttpMethod::GET, url);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    Request(request, callback, progress_callback);
}

void HttpClient::Post(const std::string& url, const std::string& body, HttpResponseCallback callback,
                     const HttpHeaders& headers, const std::string& content_type,
                     HttpProgressCallback progress_callback) {
    HttpRequest request(HttpMethod::POST, url);
    request.SetBody(body, content_type);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    Request(request, callback, progress_callback);
}

void HttpClient::Put(const std::string& url, const std::string& body, HttpResponseCallback callback,
                    const HttpHeaders& headers, const std::string& content_type,
                    HttpProgressCallback progress_callback) {
    HttpRequest request(HttpMethod::PUT, url);
    request.SetBody(body, content_type);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    Request(request, callback, progress_callback);
}

void HttpClient::Delete(const std::string& url, HttpResponseCallback callback, 
                       const HttpHeaders& headers, HttpProgressCallback progress_callback) {
    HttpRequest request(HttpMethod::DELETE, url);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    Request(request, callback, progress_callback);
}

void HttpClient::Patch(const std::string& url, const std::string& body, HttpResponseCallback callback,
                      const HttpHeaders& headers, const std::string& content_type,
                      HttpProgressCallback progress_callback) {
    HttpRequest request(HttpMethod::PATCH, url);
    request.SetBody(body, content_type);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    Request(request, callback, progress_callback);
}

void HttpClient::Request(const HttpRequest& request, HttpResponseCallback callback, 
                        HttpProgressCallback progress_callback) {
    auto prepared_request = PrepareRequest(request);
    auto session = GetAvailableSession();
    
    if (!session) {
        if (callback) {
            boost::asio::post(executor_, [callback]() {
                callback(boost::asio::error::no_descriptors, HttpResponse{});
            });
        }
        return;
    }
    
    auto self = this;
    auto start_time = std::chrono::steady_clock::now();
    
    session->AsyncRequest(prepared_request, [self, session, callback, start_time]
                         (boost::system::error_code ec, const HttpResponse& response) {
        // Update statistics
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        bool success = !ec && response.IsSuccess();
        self->UpdateStats(success, duration, 0, response.GetBody().size()); // TODO: get actual bytes sent
        
        // Return session to pool
        self->ReturnSession(session);
        
        // Call user callback
        if (callback) {
            callback(ec, response);
        }
    }, progress_callback);
}

// Synchronous methods implementation
HttpResponse HttpClient::Get(const std::string& url, const HttpHeaders& headers,
                           std::chrono::milliseconds timeout) {
    HttpRequest request(HttpMethod::GET, url);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    return Request(request, timeout);
}

HttpResponse HttpClient::Post(const std::string& url, const std::string& body,
                            const HttpHeaders& headers, const std::string& content_type,
                            std::chrono::milliseconds timeout) {
    HttpRequest request(HttpMethod::POST, url);
    request.SetBody(body, content_type);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    return Request(request, timeout);
}

HttpResponse HttpClient::Put(const std::string& url, const std::string& body,
                           const HttpHeaders& headers, const std::string& content_type,
                           std::chrono::milliseconds timeout) {
    HttpRequest request(HttpMethod::PUT, url);
    request.SetBody(body, content_type);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    return Request(request, timeout);
}

HttpResponse HttpClient::Delete(const std::string& url, const HttpHeaders& headers,
                              std::chrono::milliseconds timeout) {
    HttpRequest request(HttpMethod::DELETE, url);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    return Request(request, timeout);
}

HttpResponse HttpClient::Patch(const std::string& url, const std::string& body,
                             const HttpHeaders& headers, const std::string& content_type,
                             std::chrono::milliseconds timeout) {
    HttpRequest request(HttpMethod::PATCH, url);
    request.SetBody(body, content_type);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    return Request(request, timeout);
}

HttpResponse HttpClient::Request(const HttpRequest& request, std::chrono::milliseconds timeout) {
    auto prepared_request = PrepareRequest(request);
    auto session = GetAvailableSession();
    
    if (!session) {
        throw HttpException{HttpErrorCode::CONNECTION_FAILED, "No available HTTP session"};
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        HttpResponse response = session->Request(prepared_request, timeout);
        
        // Update statistics
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time);
        
        bool success = response.IsSuccess();
        UpdateStats(success, duration, 0, response.GetBody().size()); // TODO: get actual bytes sent
        
        // Return session to pool
        ReturnSession(session);
        
        return response;
        
    } catch (...) {
        // Return session to pool even on exception
        ReturnSession(session);
        throw;
    }
}

// JSON convenience methods (simplified implementation)
std::future<nlohmann::json> HttpClient::GetJson(const std::string& url, const HttpHeaders& headers) {
    auto promise = std::make_shared<std::promise<nlohmann::json>>();
    auto future = promise->get_future();
    
    Get(url, [promise](boost::system::error_code ec, const HttpResponse& response) {
        if (ec) {
            promise->set_value(nlohmann::json{});
        } else {
            promise->set_value(response.GetJsonBody());
        }
    }, headers);
    
    return future;
}

std::future<nlohmann::json> HttpClient::PostJson(const std::string& url, const nlohmann::json& json, 
                                                 const HttpHeaders& headers) {
    auto promise = std::make_shared<std::promise<nlohmann::json>>();
    auto future = promise->get_future();
    
    HttpRequest request(HttpMethod::POST, url);
    request.SetJsonBody(json);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    
    Request(request, [promise](boost::system::error_code ec, const HttpResponse& response) {
        if (ec) {
            promise->set_value(nlohmann::json{});
        } else {
            promise->set_value(response.GetJsonBody());
        }
    });
    
    return future;
}

std::future<nlohmann::json> HttpClient::PutJson(const std::string& url, const nlohmann::json& json, 
                                                const HttpHeaders& headers) {
    auto promise = std::make_shared<std::promise<nlohmann::json>>();
    auto future = promise->get_future();
    
    HttpRequest request(HttpMethod::PUT, url);
    request.SetJsonBody(json);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    
    Request(request, [promise](boost::system::error_code ec, const HttpResponse& response) {
        if (ec) {
            promise->set_value(nlohmann::json{});
        } else {
            promise->set_value(response.GetJsonBody());
        }
    });
    
    return future;
}

std::future<nlohmann::json> HttpClient::PatchJson(const std::string& url, const nlohmann::json& json, 
                                                  const HttpHeaders& headers) {
    auto promise = std::make_shared<std::promise<nlohmann::json>>();
    auto future = promise->get_future();
    
    HttpRequest request(HttpMethod::PATCH, url);
    request.SetJsonBody(json);
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    
    Request(request, [promise](boost::system::error_code ec, const HttpResponse& response) {
        if (ec) {
            promise->set_value(nlohmann::json{});
        } else {
            promise->set_value(response.GetJsonBody());
        }
    });
    
    return future;
}

// File operations (simplified implementation)
void HttpClient::DownloadFile(const std::string& url, const std::string& local_path, 
                             std::function<void(boost::system::error_code, const std::string&)> callback,
                             HttpProgressCallback progress_callback, const HttpHeaders& headers) {
    Get(url, [local_path, callback](boost::system::error_code ec, const HttpResponse& response) {
        if (ec) {
            if (callback) callback(ec, "");
            return;
        }
        
        if (!response.IsSuccess()) {
            if (callback) callback(boost::asio::error::invalid_argument, "");
            return;
        }
        
        // Write to file
        std::ofstream file(local_path, std::ios::binary);
        if (!file.is_open()) {
            if (callback) callback(boost::asio::error::access_denied, "");
            return;
        }
        
        file.write(response.GetBody().data(), response.GetBody().size());
        if (file.fail()) {
            if (callback) callback(boost::asio::error::no_such_device, "");
            return;
        }
        
        if (callback) callback(boost::system::error_code{}, local_path);
    }, headers, progress_callback);
}

void HttpClient::UploadFile(const std::string& url, const std::string& file_path, const std::string& field_name,
                           HttpResponseCallback callback, HttpProgressCallback progress_callback,
                           const HttpHeaders& headers) {
    // Read file content
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        if (callback) {
            boost::asio::post(executor_, [callback]() {
                callback(boost::asio::error::not_found, HttpResponse{});
            });
        }
        return;
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    
    // Create multipart request
    HttpRequest request(HttpMethod::POST, url);
    
    // Extract filename from path
    std::string filename = file_path;
    size_t last_slash = filename.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        filename = filename.substr(last_slash + 1);
    }
    
    request.AddFileField(field_name, filename, content, HttpUtils::GetMimeType(filename));
    
    for (const auto& [name, value] : headers) {
        request.SetHeader(name, value);
    }
    
    Request(request, callback, progress_callback);
}

// Authentication methods
void HttpClient::SetBasicAuth(const std::string& username, const std::string& password) {
    // Note: This is simplified - in production, properly encode base64
    std::string credentials = username + ":" + password;
    SetGlobalHeader("Authorization", "Basic " + credentials);
}

void HttpClient::SetBearerToken(const std::string& token) {
    SetGlobalHeader("Authorization", "Bearer " + token);
}

void HttpClient::SetApiKey(const std::string& key, const std::string& header_name) {
    SetGlobalHeader(header_name, key);
}

// Statistics
HttpClient::ClientStats HttpClient::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    ClientStats current_stats = stats_;
    
    // Update active sessions count
    std::lock_guard<std::mutex> session_lock(const_cast<std::mutex&>(session_mutex_));
    current_stats.active_sessions = session_pool_.size() - available_sessions_.size();
    
    return current_stats;
}

void HttpClient::CancelAllRequests() {
    std::lock_guard<std::mutex> lock(session_mutex_);
    for (auto& session : session_pool_) {
        session->Cancel();
    }
}

void HttpClient::WaitForCompletion(std::chrono::milliseconds timeout) {
    auto start_time = std::chrono::steady_clock::now();
    
    while (std::chrono::steady_clock::now() - start_time < timeout) {
        {
            std::lock_guard<std::mutex> lock(session_mutex_);
            if (available_sessions_.size() == session_pool_.size()) {
                break; // All sessions are available (not busy)
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// Private methods
std::shared_ptr<HttpSession> HttpClient::GetAvailableSession() {
    std::unique_lock<std::mutex> lock(session_mutex_);
    
    // Wait for available session with timeout
    if (session_cv_.wait_for(lock, std::chrono::milliseconds(1000), 
                            [this] { return !available_sessions_.empty() || shutdown_requested_; })) {
        
        if (shutdown_requested_ || available_sessions_.empty()) {
            return nullptr;
        }
        
        auto session = available_sessions_.front();
        available_sessions_.pop();
        return session;
    }
    
    // Timeout or no available sessions
    if (session_pool_.size() < max_pool_size_) {
        CreateSession();
        if (!available_sessions_.empty()) {
            auto session = available_sessions_.front();
            available_sessions_.pop();
            return session;
        }
    }
    
    return nullptr;
}

void HttpClient::ReturnSession(std::shared_ptr<HttpSession> session) {
    if (!session) return;
    
    std::lock_guard<std::mutex> lock(session_mutex_);
    available_sessions_.push(session);
    session_cv_.notify_one();
}

void HttpClient::CreateSession() {
    auto session = std::make_shared<HttpSession>(executor_, config_);
    session_pool_.push_back(session);
    available_sessions_.push(session);
}

HttpRequest HttpClient::PrepareRequest(const HttpRequest& original_request) const {
    HttpRequest request = original_request;
    
    // Add global headers
    for (const auto& [name, value] : global_headers_) {
        if (!request.HasHeader(name)) {
            request.SetHeader(name, value);
        }
    }
    
    // Add global cookies
    for (const auto& cookie : global_cookies_) {
        request.AddCookie(cookie);
    }
    
    // Add default headers
    if (!request.HasHeader("User-Agent")) {
        request.SetHeader("User-Agent", config_.user_agent);
    }
    
    return request;
}

void HttpClient::UpdateStats(bool success, std::chrono::milliseconds duration, size_t bytes_sent, size_t bytes_received) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    stats_.total_requests++;
    if (success) {
        stats_.successful_requests++;
    } else {
        stats_.failed_requests++;
    }
    
    stats_.total_bytes_sent += bytes_sent;
    stats_.total_bytes_received += bytes_received;
    
    // Update average request time
    double total_time = stats_.average_request_time_ms * (stats_.total_requests - 1) + duration.count();
    stats_.average_request_time_ms = total_time / stats_.total_requests;
}

// HttpClientBuilder implementation
std::unique_ptr<HttpClient> HttpClientBuilder::Build() {
    std::unique_ptr<HttpClient> client;
    
    if (executor_) {
        client = std::make_unique<HttpClient>(*executor_, config_);
    } else {
        client = std::make_unique<HttpClient>(thread_count_, config_);
    }
    
    // Set global headers
    client->SetGlobalHeaders(headers_);
    
    // Set global cookies
    client->SetGlobalCookies(cookies_);
    
    // Set authentication
    if (!basic_auth_username_.empty()) {
        client->SetBasicAuth(basic_auth_username_, basic_auth_password_);
    }
    if (!bearer_token_.empty()) {
        client->SetBearerToken(bearer_token_);
    }
    if (!api_key_.empty()) {
        client->SetApiKey(api_key_, api_key_header_);
    }
    
    return client;
}

} // namespace http
} // namespace network
} // namespace common