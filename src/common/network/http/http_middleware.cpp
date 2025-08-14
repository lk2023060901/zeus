#include "common/network/http/http_middleware.h"
#include "common/network/network_logger.h"
// #include <base64.h> // Temporarily disabled - not a standard library
// #include <zlib.h> // Temporarily disabled - external dependency
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace common {
namespace network {
namespace http {

// ===== MiddlewareManager Implementation =====

std::unordered_map<std::string, MiddlewareCreator> MiddlewareManager::creators_;
std::mutex MiddlewareManager::mutex_;

void MiddlewareManager::RegisterMiddleware(const std::string& name, MiddlewareCreator creator) {
    std::lock_guard<std::mutex> lock(mutex_);
    creators_[name] = std::move(creator);
}

std::unique_ptr<HttpMiddlewareBase> MiddlewareManager::CreateMiddleware(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = creators_.find(name);
    if (it != creators_.end()) {
        return it->second();
    }
    return nullptr;
}

std::vector<std::string> MiddlewareManager::GetRegisteredMiddlewares() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& pair : creators_) {
        names.push_back(pair.first);
    }
    return names;
}

// ===== LoggingMiddleware Implementation =====

LoggingMiddleware::LoggingMiddleware(const LogConfig& config) : config_(config) {}

void LoggingMiddleware::Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    auto start_time = std::chrono::steady_clock::now();
    
    // 记录请求开始
    if (config_.log_request_headers || config_.log_request_body) {
        std::ostringstream log_stream;
        log_stream << "HTTP Request - " << static_cast<int>(request.GetMethod()) 
                   << " " << request.GetUrl().path;
        
        if (config_.log_request_headers) {
            log_stream << "\nHeaders:";
            for (const auto& header : request.GetHeaders()) {
                log_stream << "\n  " << header.first << ": " << header.second;
            }
        }
        
        if (config_.log_request_body && !request.GetBody().empty()) {
            std::string body = request.GetBody();
            if (body.length() > config_.max_body_log_size) {
                body = body.substr(0, config_.max_body_log_size) + "...";
            }
            log_stream << "\nBody: " << body;
        }
        
        NETWORK_LOG_INFO("{}", log_stream.str());
    }
    
    // 执行下一个中间件
    next();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // 记录响应
    std::string log_entry = FormatLogEntry(request, response, duration);
    NETWORK_LOG_INFO("{}", log_entry);
}

std::string LoggingMiddleware::FormatLogEntry(const HttpRequest& request, const HttpResponse& response, 
                                             std::chrono::milliseconds duration) const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), config_.date_format.c_str());
    oss << " - " << static_cast<int>(request.GetMethod());
    oss << " " << request.GetUrl().path;
    oss << " " << static_cast<int>(response.GetStatusCode());
    oss << " " << response.GetBody().size() << "B";
    oss << " " << duration.count() << "ms";
    
    return oss.str();
}

// ===== CorsMiddleware Implementation =====

CorsMiddleware::CorsMiddleware(const CorsConfig& config) : config_(config) {}

void CorsMiddleware::Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    // 检查是否为预检请求
    if (request.GetMethod() == HttpMethod::OPTIONS) {
        HandlePreflightRequest(request, response);
        if (!config_.preflight_continue) {
            return;
        }
    }
    
    // 设置CORS头
    SetCorsHeaders(request, response);
    
    next();
}

bool CorsMiddleware::IsOriginAllowed(const std::string& origin) const {
    if (config_.allowed_origins.empty()) {
        return false;
    }
    
    // 检查是否允许所有源
    for (const auto& allowed : config_.allowed_origins) {
        if (allowed == "*" || allowed == origin) {
            return true;
        }
    }
    
    return false;
}

void CorsMiddleware::SetCorsHeaders(const HttpRequest& request, HttpResponse& response) const {
    std::string origin = request.GetHeader("Origin");
    
    if (!origin.empty() && IsOriginAllowed(origin)) {
        response.SetHeader("Access-Control-Allow-Origin", origin);
    } else if (std::find(config_.allowed_origins.begin(), config_.allowed_origins.end(), "*") 
               != config_.allowed_origins.end()) {
        response.SetHeader("Access-Control-Allow-Origin", "*");
    }
    
    if (config_.allow_credentials) {
        response.SetHeader("Access-Control-Allow-Credentials", "true");
    }
    
    // 设置暴露的头部
    if (!config_.exposed_headers.empty()) {
        std::string exposed = config_.exposed_headers[0];
        for (size_t i = 1; i < config_.exposed_headers.size(); ++i) {
            exposed += ", " + config_.exposed_headers[i];
        }
        response.SetHeader("Access-Control-Expose-Headers", exposed);
    }
}

void CorsMiddleware::HandlePreflightRequest(const HttpRequest& request, HttpResponse& response) const {
    response.SetStatusCode(HttpStatusCode::NO_CONTENT);
    
    // 允许的方法
    if (!config_.allowed_methods.empty()) {
        std::string methods = config_.allowed_methods[0];
        for (size_t i = 1; i < config_.allowed_methods.size(); ++i) {
            methods += ", " + config_.allowed_methods[i];
        }
        response.SetHeader("Access-Control-Allow-Methods", methods);
    }
    
    // 允许的头部
    if (!config_.allowed_headers.empty()) {
        std::string headers = config_.allowed_headers[0];
        for (size_t i = 1; i < config_.allowed_headers.size(); ++i) {
            headers += ", " + config_.allowed_headers[i];
        }
        response.SetHeader("Access-Control-Allow-Headers", headers);
    }
    
    // 缓存时间
    response.SetHeader("Access-Control-Max-Age", std::to_string(config_.max_age.count()));
}

// ===== AuthMiddleware Implementation =====

void AuthMiddleware::SetUnauthorizedResponse(HttpResponse& response) const {
    response.SetStatusCode(HttpStatusCode::UNAUTHORIZED);
    response.SetHeader("Content-Type", "text/plain");
    response.SetBody("401 Unauthorized");
}

// ===== BasicAuthMiddleware Implementation =====

BasicAuthMiddleware::BasicAuthMiddleware(CredentialValidator validator, const std::string& realm)
    : validator_(std::move(validator)), realm_(realm) {}

void BasicAuthMiddleware::Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    if (!Authenticate(request)) {
        response.SetHeader("WWW-Authenticate", "Basic realm=\"" + realm_ + "\"");
        SetUnauthorizedResponse(response);
        return;
    }
    
    next();
}

bool BasicAuthMiddleware::Authenticate(const HttpRequest& request) {
    std::string auth_header = request.GetHeader("Authorization");
    if (auth_header.empty() || auth_header.find("Basic ") != 0) {
        return false;
    }
    
    auto credentials = ParseBasicAuth(auth_header);
    if (credentials.first.empty()) {
        return false;
    }
    
    return validator_(credentials.first, credentials.second);
}

std::pair<std::string, std::string> BasicAuthMiddleware::ParseBasicAuth(const std::string& auth_header) const {
    std::string encoded = auth_header.substr(6); // 跳过 "Basic "
    
    // Base64解码（简化实现，实际应使用专业的base64库）
    // 这里需要实现base64解码，暂时用简化版本
    std::string decoded = encoded; // 实际需要base64解码
    
    auto colon_pos = decoded.find(':');
    if (colon_pos != std::string::npos) {
        return {decoded.substr(0, colon_pos), decoded.substr(colon_pos + 1)};
    }
    
    return {"", ""};
}

// ===== BearerAuthMiddleware Implementation =====

BearerAuthMiddleware::BearerAuthMiddleware(TokenValidator validator)
    : validator_(std::move(validator)) {}

void BearerAuthMiddleware::Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    if (!Authenticate(request)) {
        SetUnauthorizedResponse(response);
        return;
    }
    
    next();
}

bool BearerAuthMiddleware::Authenticate(const HttpRequest& request) {
    std::string auth_header = request.GetHeader("Authorization");
    std::string token = ExtractBearerToken(auth_header);
    
    if (token.empty()) {
        return false;
    }
    
    return validator_(token);
}

std::string BearerAuthMiddleware::ExtractBearerToken(const std::string& auth_header) const {
    if (auth_header.find("Bearer ") == 0) {
        return auth_header.substr(7); // 跳过 "Bearer "
    }
    return "";
}

// ===== ApiKeyAuthMiddleware Implementation =====

ApiKeyAuthMiddleware::ApiKeyAuthMiddleware(KeyValidator validator, const std::string& header_name)
    : validator_(std::move(validator)), header_name_(header_name) {}

void ApiKeyAuthMiddleware::Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    if (!Authenticate(request)) {
        SetUnauthorizedResponse(response);
        return;
    }
    
    next();
}

bool ApiKeyAuthMiddleware::Authenticate(const HttpRequest& request) {
    std::string api_key = request.GetHeader(header_name_);
    if (api_key.empty()) {
        return false;
    }
    
    return validator_(api_key);
}

// ===== RateLimitMiddleware Implementation =====

RateLimitMiddleware::RateLimitMiddleware(const RateLimitConfig& config) : config_(config) {}

void RateLimitMiddleware::Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    std::string key = GenerateKey(request);
    
    if (IsRateLimited(key)) {
        response.SetStatusCode(HttpStatusCode::TOO_MANY_REQUESTS);
        response.SetHeader("Content-Type", "text/plain");
        response.SetBody(config_.message);
        response.SetHeader("Retry-After", std::to_string(config_.window.count()));
        return;
    }
    
    next();
    
    // 如果配置为跳过成功请求，并且响应成功，则不计入限制
    if (config_.skip_successful_requests && 
        static_cast<int>(response.GetStatusCode()) >= 200 && 
        static_cast<int>(response.GetStatusCode()) < 300) {
        return;
    }
}

std::string RateLimitMiddleware::GenerateKey(const HttpRequest& request) const {
    if (config_.key_generator == "ip") {
        // 这里需要从请求中获取IP地址，暂时使用简化版本
        return "127.0.0.1"; // 实际需要从请求中提取IP
    } else if (config_.key_generator == "user") {
        // 从认证信息中获取用户ID
        return request.GetHeader("X-User-ID");
    } else if (config_.key_generator == "custom" && config_.custom_key_generator) {
        return config_.custom_key_generator(request);
    }
    
    return "default";
}

bool RateLimitMiddleware::IsRateLimited(const std::string& key) {
    std::lock_guard<std::mutex> lock(records_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto& record = request_records_[key];
    
    // 检查是否在时间窗口内
    if (now - record.timestamp > config_.window) {
        // 重置计数
        record.timestamp = now;
        record.count = 1;
        return false;
    }
    
    // 检查是否超过限制
    if (record.count >= config_.max_requests) {
        return true;
    }
    
    record.count++;
    return false;
}

void RateLimitMiddleware::CleanupExpiredRecords() {
    std::lock_guard<std::mutex> lock(records_mutex_);
    
    auto now = std::chrono::steady_clock::now();
    for (auto it = request_records_.begin(); it != request_records_.end();) {
        if (now - it->second.timestamp > config_.window) {
            it = request_records_.erase(it);
        } else {
            ++it;
        }
    }
}

// ===== CompressionMiddleware Implementation =====

CompressionMiddleware::CompressionMiddleware(const CompressionConfig& config) : config_(config) {}

void CompressionMiddleware::Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    next();
    
    if (!ShouldCompress(request, response)) {
        return;
    }
    
    std::string encoding = GetAcceptedEncoding(request);
    if (encoding.empty()) {
        return;
    }
    
    // 压缩响应体
    std::vector<uint8_t> body_bytes(response.GetBody().begin(), response.GetBody().end());
    std::vector<uint8_t> compressed = CompressData(body_bytes, encoding);
    
    if (!compressed.empty() && compressed.size() < body_bytes.size()) {
        response.SetBody(std::string(compressed.begin(), compressed.end()));
        response.SetHeader("Content-Encoding", encoding);
        response.SetHeader("Content-Length", std::to_string(compressed.size()));
    }
}

bool CompressionMiddleware::ShouldCompress(const HttpRequest& request, const HttpResponse& response) const {
    // 检查路径是否被排除
    std::string path = request.GetUrl().path;
    if (config_.excluded_paths.find(path) != config_.excluded_paths.end()) {
        return false;
    }
    
    // 检查响应大小
    if (response.GetBody().size() < config_.min_size) {
        return false;
    }
    
    // 检查MIME类型
    std::string content_type = response.GetHeader("Content-Type");
    auto pos = content_type.find(';');
    if (pos != std::string::npos) {
        content_type = content_type.substr(0, pos);
    }
    
    return std::find(config_.mime_types.begin(), config_.mime_types.end(), content_type) 
           != config_.mime_types.end();
}

std::string CompressionMiddleware::GetAcceptedEncoding(const HttpRequest& request) const {
    std::string accept_encoding = request.GetHeader("Accept-Encoding");
    if (accept_encoding.empty()) {
        return "";
    }
    
    // 简化的编码检查
    for (const auto& encoding : config_.encodings) {
        if (accept_encoding.find(encoding) != std::string::npos) {
            return encoding;
        }
    }
    
    return "";
}

std::vector<uint8_t> CompressionMiddleware::CompressData(const std::vector<uint8_t>& data, const std::string& encoding) const {
    // 这里需要实现实际的压缩算法（gzip, deflate等）
    // 暂时返回原数据作为占位符
    return data;
}

// ===== SecurityHeadersMiddleware Implementation =====

SecurityHeadersMiddleware::SecurityHeadersMiddleware(const SecurityConfig& config) : config_(config) {}

void SecurityHeadersMiddleware::Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) {
    next();
    
    // 设置安全头
    if (config_.enable_hsts && request.GetUrl().scheme == "https") {
        std::string hsts_value = "max-age=" + std::to_string(config_.hsts_max_age.count());
        if (config_.hsts_include_subdomains) {
            hsts_value += "; includeSubDomains";
        }
        response.SetHeader("Strict-Transport-Security", hsts_value);
    }
    
    if (config_.enable_xss_protection) {
        response.SetHeader("X-XSS-Protection", config_.xss_protection_mode);
    }
    
    if (config_.enable_content_type_options) {
        response.SetHeader("X-Content-Type-Options", "nosniff");
    }
    
    if (config_.enable_frame_options) {
        response.SetHeader("X-Frame-Options", config_.frame_options);
    }
    
    if (config_.enable_csp && !config_.csp_policy.empty()) {
        response.SetHeader("Content-Security-Policy", config_.csp_policy);
    }
    
    if (config_.enable_referrer_policy) {
        response.SetHeader("Referrer-Policy", config_.referrer_policy);
    }
    
    // 设置自定义头部
    for (const auto& header : config_.custom_headers) {
        response.SetHeader(header.first, header.second);
    }
}

// ===== 中间件工厂函数实现 =====

namespace middleware {

std::unique_ptr<LoggingMiddleware> Logging(const LoggingMiddleware::LogConfig& config) {
    return std::make_unique<LoggingMiddleware>(config);
}

std::unique_ptr<CorsMiddleware> Cors(const CorsMiddleware::CorsConfig& config) {
    return std::make_unique<CorsMiddleware>(config);
}

std::unique_ptr<BasicAuthMiddleware> BasicAuth(BasicAuthMiddleware::CredentialValidator validator, 
                                               const std::string& realm) {
    return std::make_unique<BasicAuthMiddleware>(std::move(validator), realm);
}

std::unique_ptr<BearerAuthMiddleware> BearerAuth(BearerAuthMiddleware::TokenValidator validator) {
    return std::make_unique<BearerAuthMiddleware>(std::move(validator));
}

std::unique_ptr<ApiKeyAuthMiddleware> ApiKeyAuth(ApiKeyAuthMiddleware::KeyValidator validator,
                                                 const std::string& header_name) {
    return std::make_unique<ApiKeyAuthMiddleware>(std::move(validator), header_name);
}

std::unique_ptr<RateLimitMiddleware> RateLimit(const RateLimitMiddleware::RateLimitConfig& config) {
    return std::make_unique<RateLimitMiddleware>(config);
}

std::unique_ptr<CompressionMiddleware> Compression(const CompressionMiddleware::CompressionConfig& config) {
    return std::make_unique<CompressionMiddleware>(config);
}

std::unique_ptr<SecurityHeadersMiddleware> Security(const SecurityHeadersMiddleware::SecurityConfig& config) {
    return std::make_unique<SecurityHeadersMiddleware>(config);
}

} // namespace middleware

} // namespace http
} // namespace network
} // namespace common