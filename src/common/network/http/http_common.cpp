#include "common/network/http/http_common.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <random>

namespace common {
namespace network {
namespace http {

// HttpUrl Implementation
HttpUrl::HttpUrl(const std::string& url) {
    *this = HttpUtils::ParseUrl(url);
}

std::string HttpUrl::ToString() const {
    std::ostringstream oss;
    
    if (!scheme.empty()) {
        oss << scheme << "://";
    }
    
    if (!host.empty()) {
        oss << host;
        
        uint16_t default_port = GetDefaultPort();
        if (port != 0 && port != default_port) {
            oss << ":" << port;
        }
    }
    
    if (!path.empty()) {
        if (path[0] != '/') {
            oss << '/';
        }
        oss << path;
    } else if (!host.empty()) {
        oss << '/';
    }
    
    if (!query.empty()) {
        oss << '?' << query;
    }
    
    if (!fragment.empty()) {
        oss << '#' << fragment;
    }
    
    return oss.str();
}

bool HttpUrl::IsValid() const {
    return !scheme.empty() && !host.empty();
}

uint16_t HttpUrl::GetDefaultPort() const {
    if (scheme == "http") return 80;
    if (scheme == "https") return 443;
    return 0;
}

// HttpUtils Implementation
namespace HttpUtils {

std::string MethodToString(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        case HttpMethod::PATCH: return "PATCH";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::TRACE: return "TRACE";
        case HttpMethod::CONNECT: return "CONNECT";
        default: return "GET";
    }
}

HttpMethod StringToMethod(const std::string& method) {
    std::string upper_method = method;
    std::transform(upper_method.begin(), upper_method.end(), upper_method.begin(), ::toupper);
    
    if (upper_method == "GET") return HttpMethod::GET;
    if (upper_method == "POST") return HttpMethod::POST;
    if (upper_method == "PUT") return HttpMethod::PUT;
    if (upper_method == "DELETE") return HttpMethod::DELETE;
    if (upper_method == "PATCH") return HttpMethod::PATCH;
    if (upper_method == "HEAD") return HttpMethod::HEAD;
    if (upper_method == "OPTIONS") return HttpMethod::OPTIONS;
    if (upper_method == "TRACE") return HttpMethod::TRACE;
    if (upper_method == "CONNECT") return HttpMethod::CONNECT;
    
    return HttpMethod::CUSTOM;
}

std::string StatusCodeToString(HttpStatusCode code) {
    switch (code) {
        case HttpStatusCode::CONTINUE: return "Continue";
        case HttpStatusCode::SWITCHING_PROTOCOLS: return "Switching Protocols";
        case HttpStatusCode::OK: return "OK";
        case HttpStatusCode::CREATED: return "Created";
        case HttpStatusCode::ACCEPTED: return "Accepted";
        case HttpStatusCode::NO_CONTENT: return "No Content";
        case HttpStatusCode::PARTIAL_CONTENT: return "Partial Content";
        case HttpStatusCode::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatusCode::FOUND: return "Found";
        case HttpStatusCode::SEE_OTHER: return "See Other";
        case HttpStatusCode::NOT_MODIFIED: return "Not Modified";
        case HttpStatusCode::TEMPORARY_REDIRECT: return "Temporary Redirect";
        case HttpStatusCode::PERMANENT_REDIRECT: return "Permanent Redirect";
        case HttpStatusCode::BAD_REQUEST: return "Bad Request";
        case HttpStatusCode::UNAUTHORIZED: return "Unauthorized";
        case HttpStatusCode::FORBIDDEN: return "Forbidden";
        case HttpStatusCode::NOT_FOUND: return "Not Found";
        case HttpStatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatusCode::NOT_ACCEPTABLE: return "Not Acceptable";
        case HttpStatusCode::CONFLICT: return "Conflict";
        case HttpStatusCode::GONE: return "Gone";
        case HttpStatusCode::PAYLOAD_TOO_LARGE: return "Payload Too Large";
        case HttpStatusCode::UNSUPPORTED_MEDIA_TYPE: return "Unsupported Media Type";
        case HttpStatusCode::TOO_MANY_REQUESTS: return "Too Many Requests";
        case HttpStatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatusCode::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatusCode::BAD_GATEWAY: return "Bad Gateway";
        case HttpStatusCode::SERVICE_UNAVAILABLE: return "Service Unavailable";
        case HttpStatusCode::GATEWAY_TIMEOUT: return "Gateway Timeout";
        case HttpStatusCode::HTTP_VERSION_NOT_SUPPORTED: return "HTTP Version Not Supported";
        default: return "Unknown Status";
    }
}

HttpUrl ParseUrl(const std::string& url) {
    HttpUrl result;
    
    // Regular expression to parse URL components
    // scheme://[userinfo@]host[:port][/path][?query][#fragment]
    std::regex url_regex(R"(^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)", 
                         std::regex_constants::icase);
    
    std::smatch matches;
    if (std::regex_match(url, matches, url_regex)) {
        // Extract scheme
        if (matches[2].matched) {
            result.scheme = matches[2].str();
            std::transform(result.scheme.begin(), result.scheme.end(), 
                          result.scheme.begin(), ::tolower);
        }
        
        // Extract host and port
        if (matches[4].matched) {
            std::string authority = matches[4].str();
            
            // Check for port
            size_t colon_pos = authority.find_last_of(':');
            if (colon_pos != std::string::npos) {
                try {
                    std::string port_str = authority.substr(colon_pos + 1);
                    result.port = static_cast<uint16_t>(std::stoi(port_str));
                    result.host = authority.substr(0, colon_pos);
                } catch (...) {
                    result.host = authority;
                }
            } else {
                result.host = authority;
            }
        }
        
        // Extract path
        if (matches[5].matched) {
            result.path = matches[5].str();
        }
        
        // Extract query
        if (matches[7].matched) {
            result.query = matches[7].str();
        }
        
        // Extract fragment
        if (matches[9].matched) {
            result.fragment = matches[9].str();
        }
    }
    
    // Set default port if not specified
    if (result.port == 0) {
        result.port = result.GetDefaultPort();
    }
    
    return result;
}

std::string BuildUrl(const std::string& base_url, const HttpParams& params) {
    if (params.empty()) {
        return base_url;
    }
    
    std::string result = base_url;
    std::string query = BuildQueryString(params);
    
    if (!query.empty()) {
        char separator = (base_url.find('?') != std::string::npos) ? '&' : '?';
        result += separator + query;
    }
    
    return result;
}

std::string UrlEncode(const std::string& input) {
    std::ostringstream oss;
    oss << std::hex << std::uppercase;
    
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            oss << c;
        } else {
            oss << '%' << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(c);
        }
    }
    
    return oss.str();
}

std::string UrlDecode(const std::string& input) {
    std::string result;
    result.reserve(input.length());
    
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '%' && i + 2 < input.length()) {
            try {
                int value = std::stoi(input.substr(i + 1, 2), nullptr, 16);
                result += static_cast<char>(value);
                i += 2;
            } catch (...) {
                result += input[i];
            }
        } else if (input[i] == '+') {
            result += ' ';
        } else {
            result += input[i];
        }
    }
    
    return result;
}

HttpParams ParseQueryString(const std::string& query) {
    HttpParams params;
    
    if (query.empty()) {
        return params;
    }
    
    std::istringstream stream(query);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t equals_pos = pair.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = UrlDecode(pair.substr(0, equals_pos));
            std::string value = UrlDecode(pair.substr(equals_pos + 1));
            params[key] = value;
        } else {
            params[UrlDecode(pair)] = "";
        }
    }
    
    return params;
}

std::string BuildQueryString(const HttpParams& params) {
    if (params.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& [key, value] : params) {
        if (!first) {
            oss << '&';
        }
        first = false;
        
        oss << UrlEncode(key);
        if (!value.empty()) {
            oss << '=' << UrlEncode(value);
        }
    }
    
    return oss.str();
}

std::vector<HttpCookie> ParseCookies(const std::string& cookie_header) {
    std::vector<HttpCookie> cookies;
    
    // Simple cookie parsing - in production, use more robust parser
    std::istringstream stream(cookie_header);
    std::string cookie_pair;
    
    while (std::getline(stream, cookie_pair, ';')) {
        // Trim whitespace
        cookie_pair.erase(0, cookie_pair.find_first_not_of(" \t"));
        cookie_pair.erase(cookie_pair.find_last_not_of(" \t") + 1);
        
        size_t equals_pos = cookie_pair.find('=');
        if (equals_pos != std::string::npos) {
            HttpCookie cookie;
            cookie.name = cookie_pair.substr(0, equals_pos);
            cookie.value = cookie_pair.substr(equals_pos + 1);
            cookies.push_back(cookie);
        }
    }
    
    return cookies;
}

std::string BuildCookieHeader(const std::vector<HttpCookie>& cookies) {
    if (cookies.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& cookie : cookies) {
        if (!first) {
            oss << "; ";
        }
        first = false;
        oss << cookie.name << "=" << cookie.value;
    }
    
    return oss.str();
}

HttpUtils::ContentType ParseContentType(const std::string& content_type) {
    ContentType result;
    
    if (content_type.empty()) {
        return result;
    }
    
    // Split by semicolon to separate media type from parameters
    size_t semicolon_pos = content_type.find(';');
    result.media_type = content_type.substr(0, semicolon_pos);
    
    // Trim whitespace
    result.media_type.erase(0, result.media_type.find_first_not_of(" \t"));
    result.media_type.erase(result.media_type.find_last_not_of(" \t") + 1);
    
    // Parse parameters
    if (semicolon_pos != std::string::npos) {
        std::string params_str = content_type.substr(semicolon_pos + 1);
        std::istringstream stream(params_str);
        std::string param;
        
        while (std::getline(stream, param, ';')) {
            param.erase(0, param.find_first_not_of(" \t"));
            param.erase(param.find_last_not_of(" \t") + 1);
            
            size_t equals_pos = param.find('=');
            if (equals_pos != std::string::npos) {
                std::string key = param.substr(0, equals_pos);
                std::string value = param.substr(equals_pos + 1);
                
                // Remove quotes from value
                if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.length() - 2);
                }
                
                if (key == "charset") {
                    result.charset = value;
                } else {
                    result.parameters[key] = value;
                }
            }
        }
    }
    
    return result;
}

std::string BuildContentType(const ContentType& content_type) {
    std::string result = content_type.media_type;
    
    if (!content_type.charset.empty()) {
        result += "; charset=" + content_type.charset;
    }
    
    for (const auto& [key, value] : content_type.parameters) {
        result += "; " + key + "=" + value;
    }
    
    return result;
}

std::string GetMimeType(const std::string& file_extension) {
    static const std::unordered_map<std::string, std::string> mime_types = {
        // Text
        {".txt", "text/plain"},
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "text/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        
        // Images
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".bmp", "image/bmp"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        
        // Audio
        {".mp3", "audio/mpeg"},
        {".wav", "audio/wav"},
        {".ogg", "audio/ogg"},
        
        // Video
        {".mp4", "video/mp4"},
        {".avi", "video/x-msvideo"},
        {".mov", "video/quicktime"},
        
        // Archives
        {".zip", "application/zip"},
        {".tar", "application/x-tar"},
        {".gz", "application/gzip"},
        
        // Documents
        {".pdf", "application/pdf"},
        {".doc", "application/msword"},
        {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    };
    
    std::string ext = file_extension;
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    auto it = mime_types.find(ext);
    return (it != mime_types.end()) ? it->second : "application/octet-stream";
}

bool IsJsonContentType(const std::string& content_type) {
    ContentType ct = ParseContentType(content_type);
    return ct.media_type == "application/json" || ct.media_type == "text/json";
}

bool IsFormContentType(const std::string& content_type) {
    ContentType ct = ParseContentType(content_type);
    return ct.media_type == "application/x-www-form-urlencoded";
}

bool IsMultipartContentType(const std::string& content_type) {
    ContentType ct = ParseContentType(content_type);
    return ct.media_type.substr(0, 10) == "multipart/";
}

std::string GenerateBoundary() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::ostringstream oss;
    oss << "----formdata-zeus-";
    
    for (int i = 0; i < 16; ++i) {
        oss << std::hex << dis(gen);
    }
    
    return oss.str();
}

std::string FormatHttpDate(const std::chrono::system_clock::time_point& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    auto tm = *std::gmtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
    return oss.str();
}

std::chrono::system_clock::time_point ParseHttpDate(const std::string& date_str) {
    std::tm tm = {};
    std::istringstream ss(date_str);
    
    // Try different date formats
    ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S");
    if (ss.fail()) {
        ss.clear();
        ss.str(date_str);
        ss >> std::get_time(&tm, "%A, %d-%b-%y %H:%M:%S");
    }
    if (ss.fail()) {
        ss.clear();
        ss.str(date_str);
        ss >> std::get_time(&tm, "%a %b %d %H:%M:%S %Y");
    }
    
    if (!ss.fail()) {
        auto time_t = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(time_t);
    }
    
    return std::chrono::system_clock::now();
}

HttpMethod BeastMethodToEnum(http::verb method) {
    switch (method) {
        case http::verb::get: return HttpMethod::GET;
        case http::verb::post: return HttpMethod::POST;
        case http::verb::put: return HttpMethod::PUT;
        case http::verb::delete_: return HttpMethod::DELETE;
        case http::verb::patch: return HttpMethod::PATCH;
        case http::verb::head: return HttpMethod::HEAD;
        case http::verb::options: return HttpMethod::OPTIONS;
        case http::verb::trace: return HttpMethod::TRACE;
        case http::verb::connect: return HttpMethod::CONNECT;
        default: return HttpMethod::CUSTOM;
    }
}

http::verb EnumToBeastMethod(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return http::verb::get;
        case HttpMethod::POST: return http::verb::post;
        case HttpMethod::PUT: return http::verb::put;
        case HttpMethod::DELETE: return http::verb::delete_;
        case HttpMethod::PATCH: return http::verb::patch;
        case HttpMethod::HEAD: return http::verb::head;
        case HttpMethod::OPTIONS: return http::verb::options;
        case HttpMethod::TRACE: return http::verb::trace;
        case HttpMethod::CONNECT: return http::verb::connect;
        default: return http::verb::get;
    }
}

HttpStatusCode BeastStatusToEnum(http::status status) {
    return static_cast<HttpStatusCode>(static_cast<int>(status));
}

http::status EnumToBeastStatus(HttpStatusCode status) {
    return static_cast<http::status>(static_cast<int>(status));
}

} // namespace HttpUtils

} // namespace http
} // namespace network
} // namespace common