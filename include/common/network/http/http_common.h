#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

namespace common {
namespace network {
namespace http {

// Type aliases for convenience
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

/**
 * @brief HTTP method enumeration
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS,
    TRACE,
    CONNECT,
    CUSTOM
};

/**
 * @brief HTTP version enumeration
 */
enum class HttpVersion {
    HTTP_1_0,
    HTTP_1_1,
    HTTP_2_0  // Future support
};

/**
 * @brief HTTP status code enumeration (commonly used)
 */
enum class HttpStatusCode {
    // 1xx Informational
    CONTINUE = 100,
    SWITCHING_PROTOCOLS = 101,
    
    // 2xx Success
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    PARTIAL_CONTENT = 206,
    
    // 3xx Redirection
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    TEMPORARY_REDIRECT = 307,
    PERMANENT_REDIRECT = 308,
    
    // 4xx Client Error
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    NOT_ACCEPTABLE = 406,
    CONFLICT = 409,
    GONE = 410,
    PAYLOAD_TOO_LARGE = 413,
    UNSUPPORTED_MEDIA_TYPE = 415,
    TOO_MANY_REQUESTS = 429,
    
    // 5xx Server Error
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505
};

/**
 * @brief HTTP header collection type
 */
using HttpHeaders = std::unordered_map<std::string, std::string>;

/**
 * @brief HTTP query parameters type
 */
using HttpParams = std::unordered_map<std::string, std::string>;

/**
 * @brief HTTP cookie structure
 */
struct HttpCookie {
    std::string name;
    std::string value;
    std::string domain;
    std::string path;
    std::chrono::system_clock::time_point expires;
    bool secure = false;
    bool http_only = false;
    std::string same_site; // "Strict", "Lax", "None"
    
    HttpCookie() = default;
    HttpCookie(const std::string& name, const std::string& value)
        : name(name), value(value) {}
};

/**
 * @brief HTTP URL structure
 */
struct HttpUrl {
    std::string scheme;     // "http" or "https"
    std::string host;       // hostname or IP
    uint16_t port = 0;      // port number (0 = default)
    std::string path;       // path component
    std::string query;      // query string
    std::string fragment;   // fragment/anchor
    
    HttpUrl() = default;
    explicit HttpUrl(const std::string& url);
    
    std::string ToString() const;
    bool IsValid() const;
    bool IsSecure() const { return scheme == "https"; }
    uint16_t GetDefaultPort() const;
};

/**
 * @brief HTTP request/response configuration
 */
struct HttpConfig {
    // Timeouts
    std::chrono::milliseconds connect_timeout{10000};     // 10 seconds
    std::chrono::milliseconds request_timeout{30000};     // 30 seconds
    std::chrono::milliseconds idle_timeout{60000};        // 60 seconds
    
    // Connection settings
    bool keep_alive = true;
    size_t max_redirects = 5;
    bool verify_ssl = true;
    std::string user_agent = "Zeus-HTTP/1.0";
    
    // Content settings
    bool auto_decompress = true;
    size_t max_response_size = 100 * 1024 * 1024; // 100 MB
    size_t buffer_size = 8192;
    
    // Retry settings
    size_t max_retries = 3;
    std::chrono::milliseconds retry_delay{1000};
    
    HttpConfig() = default;
};

/**
 * @brief HTTP progress information
 */
struct HttpProgress {
    size_t bytes_uploaded = 0;
    size_t bytes_downloaded = 0;
    size_t total_upload_size = 0;
    size_t total_download_size = 0;
    std::chrono::steady_clock::time_point start_time;
    
    double GetUploadProgress() const {
        return total_upload_size > 0 ? 
               static_cast<double>(bytes_uploaded) / total_upload_size : 0.0;
    }
    
    double GetDownloadProgress() const {
        return total_download_size > 0 ? 
               static_cast<double>(bytes_downloaded) / total_download_size : 0.0;
    }
    
    double GetSpeedBytesPerSecond() const {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        auto seconds = std::chrono::duration<double>(elapsed).count();
        return seconds > 0 ? (bytes_downloaded + bytes_uploaded) / seconds : 0.0;
    }
};

/**
 * @brief HTTP utility functions
 */
namespace HttpUtils {
    /**
     * @brief Convert HttpMethod enum to string
     */
    std::string MethodToString(HttpMethod method);
    
    /**
     * @brief Convert string to HttpMethod enum
     */
    HttpMethod StringToMethod(const std::string& method);
    
    /**
     * @brief Convert HttpStatusCode to string description
     */
    std::string StatusCodeToString(HttpStatusCode code);
    
    /**
     * @brief Parse URL into components
     */
    HttpUrl ParseUrl(const std::string& url);
    
    /**
     * @brief Build URL from components
     */
    std::string BuildUrl(const std::string& base_url, const HttpParams& params);
    
    /**
     * @brief URL encode string
     */
    std::string UrlEncode(const std::string& input);
    
    /**
     * @brief URL decode string
     */
    std::string UrlDecode(const std::string& input);
    
    /**
     * @brief Parse query string into parameters
     */
    HttpParams ParseQueryString(const std::string& query);
    
    /**
     * @brief Build query string from parameters
     */
    std::string BuildQueryString(const HttpParams& params);
    
    /**
     * @brief Parse HTTP cookie header
     */
    std::vector<HttpCookie> ParseCookies(const std::string& cookie_header);
    
    /**
     * @brief Build cookie header string
     */
    std::string BuildCookieHeader(const std::vector<HttpCookie>& cookies);
    
    /**
     * @brief Parse Content-Type header
     */
    struct ContentType {
        std::string media_type;
        std::string charset;
        std::unordered_map<std::string, std::string> parameters;
    };
    ContentType ParseContentType(const std::string& content_type);
    
    /**
     * @brief Build Content-Type header
     */
    std::string BuildContentType(const ContentType& content_type);
    
    /**
     * @brief Get MIME type from file extension
     */
    std::string GetMimeType(const std::string& file_extension);
    
    /**
     * @brief Check if content type is JSON
     */
    bool IsJsonContentType(const std::string& content_type);
    
    /**
     * @brief Check if content type is form data
     */
    bool IsFormContentType(const std::string& content_type);
    
    /**
     * @brief Check if content type is multipart
     */
    bool IsMultipartContentType(const std::string& content_type);
    
    /**
     * @brief Generate boundary string for multipart content
     */
    std::string GenerateBoundary();
    
    /**
     * @brief Format HTTP date
     */
    std::string FormatHttpDate(const std::chrono::system_clock::time_point& time);
    
    /**
     * @brief Parse HTTP date
     */
    std::chrono::system_clock::time_point ParseHttpDate(const std::string& date_str);
    
    /**
     * @brief Convert Beast HTTP method to our enum
     */
    HttpMethod BeastMethodToEnum(http::verb method);
    
    /**
     * @brief Convert our enum to Beast HTTP method
     */
    http::verb EnumToBeastMethod(HttpMethod method);
    
    /**
     * @brief Convert Beast status to our enum
     */
    HttpStatusCode BeastStatusToEnum(http::status status);
    
    /**
     * @brief Convert our enum to Beast status
     */
    http::status EnumToBeastStatus(HttpStatusCode status);
}

/**
 * @brief HTTP error categories
 */
enum class HttpErrorCode {
    // Connection errors
    CONNECTION_FAILED,
    CONNECTION_TIMEOUT,
    CONNECTION_REFUSED,
    CONNECTION_RESET,
    
    // Request errors
    INVALID_URL,
    INVALID_REQUEST,
    REQUEST_TIMEOUT,
    REQUEST_TOO_LARGE,
    
    // Response errors
    INVALID_RESPONSE,
    RESPONSE_TOO_LARGE,
    UNSUPPORTED_RESPONSE,
    
    // Protocol errors
    PROTOCOL_ERROR,
    VERSION_NOT_SUPPORTED,
    
    // SSL/TLS errors
    SSL_HANDSHAKE_FAILED,
    SSL_CERTIFICATE_ERROR,
    SSL_VERIFICATION_FAILED,
    
    // Redirect errors
    TOO_MANY_REDIRECTS,
    REDIRECT_LOOP,
    
    // Generic errors
    UNKNOWN_ERROR,
    OPERATION_CANCELLED
};

/**
 * @brief HTTP exception class
 */
class HttpException : public std::exception {
public:
    explicit HttpException(HttpErrorCode code, const std::string& message = "")
        : error_code_(code), message_(message) {}
    
    const char* what() const noexcept override {
        return message_.c_str();
    }
    
    HttpErrorCode GetErrorCode() const { return error_code_; }
    const std::string& GetMessage() const { return message_; }

private:
    HttpErrorCode error_code_;
    std::string message_;
};

// Forward declarations for main classes
class HttpRequest;
class HttpResponse;
class HttpClient;
class HttpServer;
class HttpSession;

/**
 * @brief HTTP middleware type definition
 */
using HttpMiddleware = std::function<void(const HttpRequest&, HttpResponse&, std::function<void()>)>;

/**
 * @brief HTTP request handler type definition (with middleware support)
 */
using HttpRequestHandler = std::function<void(const HttpRequest&, HttpResponse&, std::function<void()>)>;

/**
 * @brief HTTP handler type definition (simplified version)
 */
using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&, std::function<void()>)>;

/**
 * @brief Route matching result structure
 */
struct RouteMatch {
    bool matched = false;                                    // 是否匹配
    std::unordered_map<std::string, std::string> params;    // URL参数
    std::unordered_map<std::string, std::string> queries;   // 查询参数
    std::string matched_pattern;                             // 匹配的模式
    std::string matched_path;                               // 匹配的路径
};

// Other callback type definitions
using HttpResponseCallback = std::function<void(boost::system::error_code, const HttpResponse&)>;
using HttpProgressCallback = std::function<void(const HttpProgress&)>;
using HttpErrorCallback = std::function<void(const HttpException&)>;

} // namespace http
} // namespace network  
} // namespace common