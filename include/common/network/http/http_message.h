#pragma once

#include "http_common.h"
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace common {
namespace network {
namespace http {

/**
 * @brief HTTP request body types
 */
enum class HttpBodyType {
    EMPTY,
    TEXT,
    JSON,
    FORM_DATA,
    MULTIPART,
    BINARY,
    STREAM
};

/**
 * @brief HTTP multipart form field
 */
struct HttpFormField {
    std::string name;
    std::string value;
    std::string content_type;
    std::string filename;
    HttpHeaders headers;
    
    HttpFormField() = default;
    HttpFormField(const std::string& field_name, const std::string& field_value)
        : name(field_name), value(field_value) {}
};

/**
 * @brief HTTP request class
 */
class HttpRequest {
public:
    /**
     * @brief Default constructor
     */
    HttpRequest() = default;
    
    /**
     * @brief Constructor with method and URL
     */
    HttpRequest(HttpMethod method, const std::string& url);
    
    /**
     * @brief Constructor with method and HttpUrl
     */
    HttpRequest(HttpMethod method, const HttpUrl& url);
    
    /**
     * @brief Copy constructor
     */
    HttpRequest(const HttpRequest& other) = default;
    
    /**
     * @brief Move constructor
     */
    HttpRequest(HttpRequest&& other) noexcept = default;
    
    /**
     * @brief Assignment operator
     */
    HttpRequest& operator=(const HttpRequest& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    HttpRequest& operator=(HttpRequest&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~HttpRequest() = default;
    
    // Method and URL
    HttpMethod GetMethod() const { return method_; }
    void SetMethod(HttpMethod method) { method_ = method; }
    void SetMethod(const std::string& method) { method_ = HttpUtils::StringToMethod(method); }
    
    const HttpUrl& GetUrl() const { return url_; }
    void SetUrl(const std::string& url) { url_ = HttpUrl(url); }
    void SetUrl(const HttpUrl& url) { url_ = url; }
    
    // HTTP version
    HttpVersion GetVersion() const { return version_; }
    void SetVersion(HttpVersion version) { version_ = version; }
    
    // Headers
    const HttpHeaders& GetHeaders() const { return headers_; }
    void SetHeaders(const HttpHeaders& headers) { headers_ = headers; }
    void SetHeader(const std::string& name, const std::string& value) { headers_[name] = value; }
    std::string GetHeader(const std::string& name) const;
    bool HasHeader(const std::string& name) const;
    void RemoveHeader(const std::string& name);
    
    // Query parameters
    const HttpParams& GetParams() const { return params_; }
    void SetParams(const HttpParams& params) { params_ = params; }
    void SetParam(const std::string& name, const std::string& value) { params_[name] = value; }
    std::string GetParam(const std::string& name) const;
    bool HasParam(const std::string& name) const;
    void RemoveParam(const std::string& name);
    
    // Cookies
    const std::vector<HttpCookie>& GetCookies() const { return cookies_; }
    void SetCookies(const std::vector<HttpCookie>& cookies) { cookies_ = cookies; }
    void AddCookie(const HttpCookie& cookie) { cookies_.push_back(cookie); }
    void AddCookie(const std::string& name, const std::string& value) { cookies_.emplace_back(name, value); }
    
    // Body content
    HttpBodyType GetBodyType() const { return body_type_; }
    
    const std::string& GetBody() const { return body_; }
    void SetBody(const std::string& body, const std::string& content_type = "");
    void SetBody(std::string&& body, const std::string& content_type = "");
    void SetBody(const std::vector<uint8_t>& body, const std::string& content_type = "application/octet-stream");
    
    // JSON body
    void SetJsonBody(const nlohmann::json& json);
    void SetJsonBody(const std::string& json);
    nlohmann::json GetJsonBody() const;
    
    // Form data
    void SetFormData(const HttpParams& form_data);
    HttpParams GetFormData() const;
    
    // Multipart form data
    void SetMultipartForm(const std::vector<HttpFormField>& fields);
    const std::vector<HttpFormField>& GetMultipartForm() const { return multipart_fields_; }
    void AddFormField(const HttpFormField& field) { multipart_fields_.push_back(field); }
    void AddFormField(const std::string& name, const std::string& value);
    void AddFileField(const std::string& name, const std::string& filename, 
                     const std::string& content, const std::string& content_type = "");
    
    // Content properties
    size_t GetContentLength() const;
    std::string GetContentType() const;
    void SetContentType(const std::string& content_type);
    
    // Authentication
    void SetBasicAuth(const std::string& username, const std::string& password);
    void SetBearerToken(const std::string& token);
    void SetApiKey(const std::string& key, const std::string& header_name = "X-API-Key");
    
    // Utility methods
    std::string BuildRequestLine() const;
    std::string BuildHeadersString() const;
    std::string ToString() const;
    
    // Beast HTTP integration
    template<class Body = boost::beast::http::string_body>
    boost::beast::http::request<Body> ToBeastRequest() const;
    
    static HttpRequest FromBeastRequest(const boost::beast::http::request<boost::beast::http::string_body>& beast_req);
    
private:
    void UpdateBodyFromMultipart();
    void UpdateBodyFromFormData();
    std::string GenerateMultipartBody() const;
    
    HttpMethod method_ = HttpMethod::GET;
    HttpUrl url_;
    HttpVersion version_ = HttpVersion::HTTP_1_1;
    HttpHeaders headers_;
    HttpParams params_;
    std::vector<HttpCookie> cookies_;
    
    HttpBodyType body_type_ = HttpBodyType::EMPTY;
    std::string body_;
    std::vector<HttpFormField> multipart_fields_;
    std::string multipart_boundary_;
};

/**
 * @brief HTTP response class
 */
class HttpResponse {
public:
    /**
     * @brief Default constructor
     */
    HttpResponse() = default;
    
    /**
     * @brief Constructor with status code
     */
    explicit HttpResponse(HttpStatusCode status_code);
    
    /**
     * @brief Copy constructor
     */
    HttpResponse(const HttpResponse& other) = default;
    
    /**
     * @brief Move constructor
     */
    HttpResponse(HttpResponse&& other) noexcept = default;
    
    /**
     * @brief Assignment operator
     */
    HttpResponse& operator=(const HttpResponse& other) = default;
    
    /**
     * @brief Move assignment operator
     */
    HttpResponse& operator=(HttpResponse&& other) noexcept = default;
    
    /**
     * @brief Destructor
     */
    ~HttpResponse() = default;
    
    // Status
    HttpStatusCode GetStatusCode() const { return status_code_; }
    void SetStatusCode(HttpStatusCode status_code) { status_code_ = status_code; }
    
    std::string GetReasonPhrase() const { return reason_phrase_; }
    void SetReasonPhrase(const std::string& reason) { reason_phrase_ = reason; }
    
    // HTTP version
    HttpVersion GetVersion() const { return version_; }
    void SetVersion(HttpVersion version) { version_ = version; }
    
    // Headers
    const HttpHeaders& GetHeaders() const { return headers_; }
    void SetHeaders(const HttpHeaders& headers) { headers_ = headers; }
    void SetHeader(const std::string& name, const std::string& value) { headers_[name] = value; }
    std::string GetHeader(const std::string& name) const;
    bool HasHeader(const std::string& name) const;
    void RemoveHeader(const std::string& name);
    
    // Cookies
    const std::vector<HttpCookie>& GetCookies() const { return cookies_; }
    void SetCookies(const std::vector<HttpCookie>& cookies) { cookies_ = cookies; }
    void AddCookie(const HttpCookie& cookie) { cookies_.push_back(cookie); }
    void SetCookie(const std::string& name, const std::string& value, 
                   const std::string& path = "/", const std::string& domain = "",
                   bool secure = false, bool http_only = true);
    
    // Body content
    const std::string& GetBody() const { return body_; }
    void SetBody(const std::string& body, const std::string& content_type = "text/plain");
    void SetBody(std::string&& body, const std::string& content_type = "text/plain");
    void SetBody(const std::vector<uint8_t>& body, const std::string& content_type = "application/octet-stream");
    
    // JSON body
    void SetJsonBody(const nlohmann::json& json);
    void SetJsonBody(const std::string& json);
    nlohmann::json GetJsonBody() const;
    
    // HTML body
    void SetHtmlBody(const std::string& html);
    
    // File response
    void SetFileBody(const std::string& file_path);
    
    // Content properties
    size_t GetContentLength() const;
    std::string GetContentType() const;
    void SetContentType(const std::string& content_type);
    
    // Status checks
    bool IsSuccess() const { return static_cast<int>(status_code_) >= 200 && static_cast<int>(status_code_) < 300; }
    bool IsRedirect() const { return static_cast<int>(status_code_) >= 300 && static_cast<int>(status_code_) < 400; }
    bool IsClientError() const { return static_cast<int>(status_code_) >= 400 && static_cast<int>(status_code_) < 500; }
    bool IsServerError() const { return static_cast<int>(status_code_) >= 500 && static_cast<int>(status_code_) < 600; }
    bool IsError() const { return IsClientError() || IsServerError(); }
    
    // Utility methods
    std::string BuildStatusLine() const;
    std::string BuildHeadersString() const;
    std::string ToString() const;
    
    // Beast HTTP integration
    template<class Body = boost::beast::http::string_body>
    boost::beast::http::response<Body> ToBeastResponse() const;
    
    static HttpResponse FromBeastResponse(const boost::beast::http::response<boost::beast::http::string_body>& beast_resp);
    
    // Convenience factory methods
    static HttpResponse Ok(const std::string& body = "", const std::string& content_type = "text/plain");
    static HttpResponse Created(const std::string& body = "", const std::string& location = "");
    static HttpResponse NoContent();
    static HttpResponse BadRequest(const std::string& message = "Bad Request");
    static HttpResponse Unauthorized(const std::string& message = "Unauthorized");
    static HttpResponse Forbidden(const std::string& message = "Forbidden");
    static HttpResponse NotFound(const std::string& message = "Not Found");
    static HttpResponse InternalServerError(const std::string& message = "Internal Server Error");
    static HttpResponse Json(const nlohmann::json& json, HttpStatusCode status = HttpStatusCode::OK);
    static HttpResponse Html(const std::string& html, HttpStatusCode status = HttpStatusCode::OK);
    static HttpResponse Redirect(const std::string& location, HttpStatusCode status = HttpStatusCode::FOUND);
    
private:
    HttpStatusCode status_code_ = HttpStatusCode::OK;
    std::string reason_phrase_;
    HttpVersion version_ = HttpVersion::HTTP_1_1;
    HttpHeaders headers_;
    std::vector<HttpCookie> cookies_;
    std::string body_;
};

// Template implementations
template<class Body>
boost::beast::http::request<Body> HttpRequest::ToBeastRequest() const {
    boost::beast::http::request<Body> req;
    
    // Set method
    req.method(HttpUtils::EnumToBeastMethod(method_));
    
    // Set target (path + query)
    std::string target = url_.path.empty() ? "/" : url_.path;
    if (!url_.query.empty()) {
        target += "?" + url_.query;
    }
    if (!params_.empty()) {
        std::string query = HttpUtils::BuildQueryString(params_);
        if (!query.empty()) {
            target += (url_.query.empty() ? "?" : "&") + query;
        }
    }
    req.target(target);
    
    // Set version
    req.version(version_ == HttpVersion::HTTP_1_0 ? 10 : 11);
    
    // Set headers
    for (const auto& [name, value] : headers_) {
        req.set(name, value);
    }
    
    // Set cookies
    if (!cookies_.empty()) {
        req.set(boost::beast::http::field::cookie, HttpUtils::BuildCookieHeader(cookies_));
    }
    
    // Set body
    if (!body_.empty()) {
        if constexpr (std::is_same_v<Body, boost::beast::http::string_body>) {
            req.body() = body_;
        }
        req.prepare_payload();
    }
    
    return req;
}

template<class Body>
boost::beast::http::response<Body> HttpResponse::ToBeastResponse() const {
    boost::beast::http::response<Body> resp;
    
    // Set status
    resp.result(HttpUtils::EnumToBeastStatus(status_code_));
    
    // Set version
    resp.version(version_ == HttpVersion::HTTP_1_0 ? 10 : 11);
    
    // Set headers
    for (const auto& [name, value] : headers_) {
        resp.set(name, value);
    }
    
    // Set cookies
    for (const auto& cookie : cookies_) {
        std::string cookie_header = cookie.name + "=" + cookie.value;
        if (!cookie.path.empty()) {
            cookie_header += "; Path=" + cookie.path;
        }
        if (!cookie.domain.empty()) {
            cookie_header += "; Domain=" + cookie.domain;
        }
        if (cookie.secure) {
            cookie_header += "; Secure";
        }
        if (cookie.http_only) {
            cookie_header += "; HttpOnly";
        }
        if (!cookie.same_site.empty()) {
            cookie_header += "; SameSite=" + cookie.same_site;
        }
        resp.set(boost::beast::http::field::set_cookie, cookie_header);
    }
    
    // Set body
    if (!body_.empty()) {
        if constexpr (std::is_same_v<Body, boost::beast::http::string_body>) {
            resp.body() = body_;
        }
        resp.prepare_payload();
    }
    
    return resp;
}

} // namespace http
} // namespace network
} // namespace common