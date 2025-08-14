#pragma once

#include "http_common.h"
#include "http_message.h"
#include "../network_logger.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <mutex>
#include <chrono>
#include <regex>

namespace common {
namespace network {
namespace http {

/**
 * @brief HTTP中间件基类
 */
class HttpMiddlewareBase {
public:
    virtual ~HttpMiddlewareBase() = default;
    
    /**
     * @brief 处理HTTP请求
     * @param request HTTP请求
     * @param response HTTP响应
     * @param next 调用下一个中间件的函数
     */
    virtual void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) = 0;
};

/**
 * @brief 中间件创建器函数类型
 */
using MiddlewareCreator = std::function<std::unique_ptr<HttpMiddlewareBase>()>;

/**
 * @brief 中间件管理器
 */
class MiddlewareManager {
public:
    /**
     * @brief 注册中间件创建器
     */
    static void RegisterMiddleware(const std::string& name, MiddlewareCreator creator);
    
    /**
     * @brief 创建中间件实例
     */
    static std::unique_ptr<HttpMiddlewareBase> CreateMiddleware(const std::string& name);
    
    /**
     * @brief 获取所有已注册的中间件名称
     */
    static std::vector<std::string> GetRegisteredMiddlewares();

private:
    static std::unordered_map<std::string, MiddlewareCreator> creators_;
    static std::mutex mutex_;
};

// ===== 内建中间件实现 =====

/**
 * @brief 日志中间件 - 记录HTTP请求和响应
 */
class LoggingMiddleware : public HttpMiddlewareBase {
public:
    struct LogConfig {
        bool log_request_headers = false;     // 记录请求头
        bool log_response_headers = false;    // 记录响应头
        bool log_request_body = false;        // 记录请求体
        bool log_response_body = false;       // 记录响应体
        size_t max_body_log_size = 1024;     // 最大记录体大小
        std::string date_format = "%Y-%m-%d %H:%M:%S"; // 日期格式
        
        LogConfig() = default;
        static LogConfig Default() { return LogConfig{}; }
    };
    
    explicit LoggingMiddleware(const LogConfig& config = LogConfig::Default());
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    LogConfig config_;
    std::string FormatLogEntry(const HttpRequest& request, const HttpResponse& response, 
                              std::chrono::milliseconds duration) const;
};

/**
 * @brief CORS中间件 - 处理跨域请求
 */
class CorsMiddleware : public HttpMiddlewareBase {
public:
    struct CorsConfig {
        std::vector<std::string> allowed_origins = {"*"};          // 允许的源
        std::vector<std::string> allowed_methods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"}; // 允许的方法
        std::vector<std::string> allowed_headers = {"*"};          // 允许的头部
        std::vector<std::string> exposed_headers;                   // 暴露的头部
        bool allow_credentials = false;                             // 允许凭据
        std::chrono::seconds max_age{86400};                       // 预检请求缓存时间
        bool preflight_continue = false;                            // 预检请求是否继续处理
        
        CorsConfig() = default;
        static CorsConfig Default() { return CorsConfig{}; }
    };
    
    explicit CorsMiddleware(const CorsConfig& config = CorsConfig::Default());
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    CorsConfig config_;
    bool IsOriginAllowed(const std::string& origin) const;
    void SetCorsHeaders(const HttpRequest& request, HttpResponse& response) const;
    void HandlePreflightRequest(const HttpRequest& request, HttpResponse& response) const;
};

/**
 * @brief 身份验证中间件基类
 */
class AuthMiddleware : public HttpMiddlewareBase {
public:
    virtual ~AuthMiddleware() = default;

protected:
    virtual bool Authenticate(const HttpRequest& request) = 0;
    virtual void SetUnauthorizedResponse(HttpResponse& response) const;
};

/**
 * @brief Basic身份验证中间件
 */
class BasicAuthMiddleware : public AuthMiddleware {
public:
    using CredentialValidator = std::function<bool(const std::string& username, const std::string& password)>;
    
    explicit BasicAuthMiddleware(CredentialValidator validator, const std::string& realm = "Protected Area");
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

protected:
    bool Authenticate(const HttpRequest& request) override;

private:
    CredentialValidator validator_;
    std::string realm_;
    std::pair<std::string, std::string> ParseBasicAuth(const std::string& auth_header) const;
};

/**
 * @brief Bearer Token身份验证中间件
 */
class BearerAuthMiddleware : public AuthMiddleware {
public:
    using TokenValidator = std::function<bool(const std::string& token)>;
    
    explicit BearerAuthMiddleware(TokenValidator validator);
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

protected:
    bool Authenticate(const HttpRequest& request) override;

private:
    TokenValidator validator_;
    std::string ExtractBearerToken(const std::string& auth_header) const;
};

/**
 * @brief API Key身份验证中间件
 */
class ApiKeyAuthMiddleware : public AuthMiddleware {
public:
    using KeyValidator = std::function<bool(const std::string& api_key)>;
    
    explicit ApiKeyAuthMiddleware(KeyValidator validator, const std::string& header_name = "X-API-Key");
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

protected:
    bool Authenticate(const HttpRequest& request) override;

private:
    KeyValidator validator_;
    std::string header_name_;
};

/**
 * @brief 速率限制中间件
 */
class RateLimitMiddleware : public HttpMiddlewareBase {
public:
    struct RateLimitConfig {
        size_t max_requests = 100;                    // 最大请求数
        std::chrono::seconds window{60};              // 时间窗口
        std::string key_generator = "ip";             // 键生成策略 ("ip", "user", "custom")
        std::function<std::string(const HttpRequest&)> custom_key_generator; // 自定义键生成器
        bool skip_successful_requests = false;        // 跳过成功请求
        std::string message = "Too Many Requests";    // 限制消息
        
        RateLimitConfig() = default;
        static RateLimitConfig Default() { return RateLimitConfig{}; }
    };
    
    explicit RateLimitMiddleware(const RateLimitConfig& config = RateLimitConfig::Default());
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    struct RequestRecord {
        std::chrono::steady_clock::time_point timestamp;
        size_t count = 1;
    };
    
    RateLimitConfig config_;
    std::unordered_map<std::string, RequestRecord> request_records_;
    std::mutex records_mutex_;
    
    std::string GenerateKey(const HttpRequest& request) const;
    bool IsRateLimited(const std::string& key);
    void CleanupExpiredRecords();
};

/**
 * @brief 压缩中间件
 */
class CompressionMiddleware : public HttpMiddlewareBase {
public:
    struct CompressionConfig {
        std::vector<std::string> encodings = {"gzip", "deflate"}; // 支持的编码
        size_t min_size = 1024;                                   // 最小压缩大小
        std::vector<std::string> mime_types = {                   // 压缩的MIME类型
            "text/html", "text/plain", "text/css", "text/javascript",
            "application/javascript", "application/json", "application/xml"
        };
        std::unordered_set<std::string> excluded_paths;           // 排除的路径
        int compression_level = 6;                                // 压缩级别 (1-9)
        
        CompressionConfig() = default;
        static CompressionConfig Default() { return CompressionConfig{}; }
    };
    
    explicit CompressionMiddleware(const CompressionConfig& config = CompressionConfig::Default());
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    CompressionConfig config_;
    
    bool ShouldCompress(const HttpRequest& request, const HttpResponse& response) const;
    std::string GetAcceptedEncoding(const HttpRequest& request) const;
    std::vector<uint8_t> CompressData(const std::vector<uint8_t>& data, const std::string& encoding) const;
};

/**
 * @brief 缓存中间件
 */
class CacheMiddleware : public HttpMiddlewareBase {
public:
    struct CacheConfig {
        std::chrono::seconds max_age{3600};           // 缓存最大年龄
        std::chrono::seconds stale_while_revalidate{60}; // 过期后重验证时间
        bool must_revalidate = false;                 // 必须重验证
        bool no_cache = false;                        // 不缓存
        bool no_store = false;                        // 不存储
        bool public_cache = true;                     // 公共缓存
        std::vector<std::string> vary_headers;        // Vary头部
        
        CacheConfig() = default;
        static CacheConfig Default() { return CacheConfig{}; }
    };
    
    explicit CacheMiddleware(const CacheConfig& config = CacheConfig::Default());
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    CacheConfig config_;
    void SetCacheHeaders(HttpResponse& response) const;
};

/**
 * @brief 安全头中间件
 */
class SecurityHeadersMiddleware : public HttpMiddlewareBase {
public:
    struct SecurityConfig {
        bool enable_hsts = true;                      // HTTP严格传输安全
        std::chrono::seconds hsts_max_age{31536000};  // HSTS最大年龄
        bool hsts_include_subdomains = true;          // HSTS包含子域
        
        bool enable_xss_protection = true;            // XSS保护
        std::string xss_protection_mode = "1; mode=block";
        
        bool enable_content_type_options = true;     // 内容类型选项
        bool enable_frame_options = true;            // 框架选项
        std::string frame_options = "DENY";          // DENY, SAMEORIGIN, ALLOW-FROM
        
        bool enable_csp = false;                     // 内容安全策略
        std::string csp_policy;                      // CSP策略
        
        bool enable_referrer_policy = true;          // 引用策略
        std::string referrer_policy = "strict-origin-when-cross-origin";
        
        std::unordered_map<std::string, std::string> custom_headers; // 自定义头部
        
        SecurityConfig() = default;
        static SecurityConfig Default() { return SecurityConfig{}; }
    };
    
    explicit SecurityHeadersMiddleware(const SecurityConfig& config = SecurityConfig::Default());
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    SecurityConfig config_;
};

/**
 * @brief 请求/响应大小限制中间件
 */
class SizeLimitMiddleware : public HttpMiddlewareBase {
public:
    struct SizeLimitConfig {
        size_t max_request_size = 10 * 1024 * 1024;  // 最大请求大小 (10MB)
        size_t max_header_size = 64 * 1024;          // 最大头部大小 (64KB)
        size_t max_url_length = 2048;                // 最大URL长度
        std::string error_message = "Request Too Large";
        
        SizeLimitConfig() = default;
        static SizeLimitConfig Default() { return SizeLimitConfig{}; }
    };
    
    explicit SizeLimitMiddleware(const SizeLimitConfig& config = SizeLimitConfig::Default());
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    SizeLimitConfig config_;
};

/**
 * @brief 超时中间件
 */
class TimeoutMiddleware : public HttpMiddlewareBase {
public:
    explicit TimeoutMiddleware(std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    std::chrono::milliseconds timeout_;
};

/**
 * @brief 错误处理中间件
 */
class ErrorHandlerMiddleware : public HttpMiddlewareBase {
public:
    using ErrorHandler = std::function<void(const std::exception&, const HttpRequest&, HttpResponse&)>;
    
    explicit ErrorHandlerMiddleware(ErrorHandler handler = nullptr);
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    ErrorHandler error_handler_;
    void DefaultErrorHandler(const std::exception& error, const HttpRequest& request, HttpResponse& response);
};

/**
 * @brief 路径重写中间件
 */
class RewriteMiddleware : public HttpMiddlewareBase {
public:
    using RewriteRule = std::pair<std::regex, std::string>; // 模式和替换
    
    explicit RewriteMiddleware(const std::vector<RewriteRule>& rules);
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override;

private:
    std::vector<RewriteRule> rules_;
};

/**
 * @brief 条件中间件 - 根据条件决定是否应用中间件
 */
template<typename T>
class ConditionalMiddleware : public HttpMiddlewareBase {
public:
    using Condition = std::function<bool(const HttpRequest&)>;
    
    ConditionalMiddleware(std::unique_ptr<T> middleware, Condition condition)
        : middleware_(std::move(middleware)), condition_(std::move(condition)) {}
    
    void Handle(const HttpRequest& request, HttpResponse& response, std::function<void()> next) override {
        if (condition_(request)) {
            middleware_->Handle(request, response, next);
        } else {
            next();
        }
    }

private:
    std::unique_ptr<T> middleware_;
    Condition condition_;
};

// ===== 中间件工厂函数 =====

namespace middleware {

/**
 * @brief 创建日志中间件
 */
std::unique_ptr<LoggingMiddleware> Logging(const LoggingMiddleware::LogConfig& config = LoggingMiddleware::LogConfig::Default());

/**
 * @brief 创建CORS中间件
 */
std::unique_ptr<CorsMiddleware> Cors(const CorsMiddleware::CorsConfig& config = CorsMiddleware::CorsConfig::Default());

/**
 * @brief 创建Basic身份验证中间件
 */
std::unique_ptr<BasicAuthMiddleware> BasicAuth(BasicAuthMiddleware::CredentialValidator validator, 
                                               const std::string& realm = "Protected Area");

/**
 * @brief 创建Bearer Token身份验证中间件
 */
std::unique_ptr<BearerAuthMiddleware> BearerAuth(BearerAuthMiddleware::TokenValidator validator);

/**
 * @brief 创建API Key身份验证中间件
 */
std::unique_ptr<ApiKeyAuthMiddleware> ApiKeyAuth(ApiKeyAuthMiddleware::KeyValidator validator,
                                                 const std::string& header_name = "X-API-Key");

/**
 * @brief 创建速率限制中间件
 */
std::unique_ptr<RateLimitMiddleware> RateLimit(const RateLimitMiddleware::RateLimitConfig& config = RateLimitMiddleware::RateLimitConfig::Default());

/**
 * @brief 创建压缩中间件
 */
std::unique_ptr<CompressionMiddleware> Compression(const CompressionMiddleware::CompressionConfig& config = CompressionMiddleware::CompressionConfig::Default());

/**
 * @brief 创建缓存中间件
 */
std::unique_ptr<CacheMiddleware> Cache(const CacheMiddleware::CacheConfig& config = CacheMiddleware::CacheConfig::Default());

/**
 * @brief 创建安全头中间件
 */
std::unique_ptr<SecurityHeadersMiddleware> Security(const SecurityHeadersMiddleware::SecurityConfig& config = SecurityHeadersMiddleware::SecurityConfig::Default());

/**
 * @brief 创建大小限制中间件
 */
std::unique_ptr<SizeLimitMiddleware> SizeLimit(const SizeLimitMiddleware::SizeLimitConfig& config = SizeLimitMiddleware::SizeLimitConfig::Default());

/**
 * @brief 创建超时中间件
 */
std::unique_ptr<TimeoutMiddleware> Timeout(std::chrono::milliseconds timeout = std::chrono::milliseconds{30000});

/**
 * @brief 创建错误处理中间件
 */
std::unique_ptr<ErrorHandlerMiddleware> ErrorHandler(ErrorHandlerMiddleware::ErrorHandler handler = nullptr);

/**
 * @brief 创建路径重写中间件
 */
std::unique_ptr<RewriteMiddleware> Rewrite(const std::vector<RewriteMiddleware::RewriteRule>& rules);

/**
 * @brief 创建条件中间件
 */
template<typename T>
std::unique_ptr<ConditionalMiddleware<T>> Conditional(std::unique_ptr<T> middleware, 
                                                      typename ConditionalMiddleware<T>::Condition condition) {
    return std::make_unique<ConditionalMiddleware<T>>(std::move(middleware), std::move(condition));
}

} // namespace middleware

} // namespace http
} // namespace network
} // namespace common