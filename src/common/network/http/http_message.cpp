#include "common/network/http/http_message.h"
#include <fstream>
#include <sstream>
// TODO: Implement or include proper Base64 library

namespace common {
namespace network {
namespace http {

// HttpRequest Implementation
HttpRequest::HttpRequest(HttpMethod method, const std::string& url)
    : method_(method), url_(url) {
}

HttpRequest::HttpRequest(HttpMethod method, const HttpUrl& url)
    : method_(method), url_(url) {
}

std::string HttpRequest::GetHeader(const std::string& name) const {
    auto it = headers_.find(name);
    return (it != headers_.end()) ? it->second : "";
}

bool HttpRequest::HasHeader(const std::string& name) const {
    return headers_.find(name) != headers_.end();
}

void HttpRequest::RemoveHeader(const std::string& name) {
    headers_.erase(name);
}

std::string HttpRequest::GetParam(const std::string& name) const {
    auto it = params_.find(name);
    return (it != params_.end()) ? it->second : "";
}

bool HttpRequest::HasParam(const std::string& name) const {
    return params_.find(name) != params_.end();
}

void HttpRequest::RemoveParam(const std::string& name) {
    params_.erase(name);
}

void HttpRequest::SetBody(const std::string& body, const std::string& content_type) {
    body_ = body;
    body_type_ = HttpBodyType::TEXT;
    if (!content_type.empty()) {
        SetContentType(content_type);
    }
}

void HttpRequest::SetBody(std::string&& body, const std::string& content_type) {
    body_ = std::move(body);
    body_type_ = HttpBodyType::TEXT;
    if (!content_type.empty()) {
        SetContentType(content_type);
    }
}

void HttpRequest::SetBody(const std::vector<uint8_t>& body, const std::string& content_type) {
    body_.assign(body.begin(), body.end());
    body_type_ = HttpBodyType::BINARY;
    SetContentType(content_type);
}

void HttpRequest::SetJsonBody(const nlohmann::json& json) {
    body_ = json.dump();
    body_type_ = HttpBodyType::JSON;
    SetContentType("application/json");
}

void HttpRequest::SetJsonBody(const std::string& json) {
    body_ = json;
    body_type_ = HttpBodyType::JSON;
    SetContentType("application/json");
}

nlohmann::json HttpRequest::GetJsonBody() const {
    try {
        return nlohmann::json::parse(body_);
    } catch (const nlohmann::json::exception&) {
        return nlohmann::json();
    }
}

void HttpRequest::SetFormData(const HttpParams& form_data) {
    body_ = HttpUtils::BuildQueryString(form_data);
    body_type_ = HttpBodyType::FORM_DATA;
    SetContentType("application/x-www-form-urlencoded");
}

HttpParams HttpRequest::GetFormData() const {
    if (body_type_ == HttpBodyType::FORM_DATA) {
        return HttpUtils::ParseQueryString(body_);
    }
    return HttpParams{};
}

void HttpRequest::SetMultipartForm(const std::vector<HttpFormField>& fields) {
    multipart_fields_ = fields;
    body_type_ = HttpBodyType::MULTIPART;
    multipart_boundary_ = HttpUtils::GenerateBoundary();
    UpdateBodyFromMultipart();
}

void HttpRequest::AddFormField(const std::string& name, const std::string& value) {
    multipart_fields_.emplace_back(name, value);
    if (body_type_ == HttpBodyType::MULTIPART) {
        UpdateBodyFromMultipart();
    }
}

void HttpRequest::AddFileField(const std::string& name, const std::string& filename, 
                              const std::string& content, const std::string& content_type) {
    HttpFormField field;
    field.name = name;
    field.filename = filename;
    field.value = content;
    field.content_type = content_type.empty() ? "application/octet-stream" : content_type;
    
    multipart_fields_.push_back(field);
    if (body_type_ == HttpBodyType::MULTIPART) {
        UpdateBodyFromMultipart();
    }
}

size_t HttpRequest::GetContentLength() const {
    return body_.length();
}

std::string HttpRequest::GetContentType() const {
    return GetHeader("Content-Type");
}

void HttpRequest::SetContentType(const std::string& content_type) {
    SetHeader("Content-Type", content_type);
}

void HttpRequest::SetBasicAuth(const std::string& username, const std::string& password) {
    std::string credentials = username + ":" + password;
    // Note: This is a placeholder. In a real implementation, you'd use a proper base64 encoder
    std::string encoded = "Basic " + credentials; // Should be base64 encoded
    SetHeader("Authorization", encoded);
}

void HttpRequest::SetBearerToken(const std::string& token) {
    SetHeader("Authorization", "Bearer " + token);
}

void HttpRequest::SetApiKey(const std::string& key, const std::string& header_name) {
    SetHeader(header_name, key);
}

std::string HttpRequest::BuildRequestLine() const {
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
    
    std::string version_str = (version_ == HttpVersion::HTTP_1_0) ? "HTTP/1.0" : "HTTP/1.1";
    return HttpUtils::MethodToString(method_) + " " + target + " " + version_str;
}

std::string HttpRequest::BuildHeadersString() const {
    std::ostringstream oss;
    
    // Add standard headers
    for (const auto& [name, value] : headers_) {
        oss << name << ": " << value << "\r\n";
    }
    
    // Add cookies
    if (!cookies_.empty()) {
        oss << "Cookie: " << HttpUtils::BuildCookieHeader(cookies_) << "\r\n";
    }
    
    return oss.str();
}

std::string HttpRequest::ToString() const {
    std::ostringstream oss;
    oss << BuildRequestLine() << "\r\n";
    oss << BuildHeadersString();
    oss << "\r\n";
    if (!body_.empty()) {
        oss << body_;
    }
    return oss.str();
}

HttpRequest HttpRequest::FromBeastRequest(const boost::beast::http::request<boost::beast::http::string_body>& beast_req) {
    HttpRequest request;
    
    // Convert method
    request.method_ = HttpUtils::BeastMethodToEnum(beast_req.method());
    
    // Parse target
    std::string target = beast_req.target();
    size_t query_pos = target.find('?');
    if (query_pos != std::string::npos) {
        request.url_.path = target.substr(0, query_pos);
        request.url_.query = target.substr(query_pos + 1);
        request.params_ = HttpUtils::ParseQueryString(request.url_.query);
    } else {
        request.url_.path = target;
    }
    
    // Convert version
    request.version_ = (beast_req.version() == 10) ? HttpVersion::HTTP_1_0 : HttpVersion::HTTP_1_1;
    
    // Convert headers
    for (const auto& field : beast_req) {
        std::string name(field.name_string());
        std::string value(field.value());
        
        if (name == "Cookie") {
            request.cookies_ = HttpUtils::ParseCookies(value);
        } else {
            request.headers_[name] = value;
        }
    }
    
    // Copy body
    request.body_ = beast_req.body();
    request.body_type_ = request.body_.empty() ? HttpBodyType::EMPTY : HttpBodyType::TEXT;
    
    return request;
}

void HttpRequest::UpdateBodyFromMultipart() {
    if (multipart_boundary_.empty()) {
        multipart_boundary_ = HttpUtils::GenerateBoundary();
    }
    
    body_ = GenerateMultipartBody();
    SetContentType("multipart/form-data; boundary=" + multipart_boundary_);
}

void HttpRequest::UpdateBodyFromFormData() {
    HttpParams form_params;
    for (const auto& field : multipart_fields_) {
        form_params[field.name] = field.value;
    }
    body_ = HttpUtils::BuildQueryString(form_params);
    SetContentType("application/x-www-form-urlencoded");
}

std::string HttpRequest::GenerateMultipartBody() const {
    std::ostringstream oss;
    
    for (const auto& field : multipart_fields_) {
        oss << "--" << multipart_boundary_ << "\r\n";
        oss << "Content-Disposition: form-data; name=\"" << field.name << "\"";
        
        if (!field.filename.empty()) {
            oss << "; filename=\"" << field.filename << "\"";
        }
        
        oss << "\r\n";
        
        if (!field.content_type.empty()) {
            oss << "Content-Type: " << field.content_type << "\r\n";
        }
        
        // Add custom headers
        for (const auto& [name, value] : field.headers) {
            oss << name << ": " << value << "\r\n";
        }
        
        oss << "\r\n";
        oss << field.value << "\r\n";
    }
    
    oss << "--" << multipart_boundary_ << "--\r\n";
    
    return oss.str();
}

// HttpResponse Implementation
HttpResponse::HttpResponse(HttpStatusCode status_code)
    : status_code_(status_code), reason_phrase_(HttpUtils::StatusCodeToString(status_code)) {
}

std::string HttpResponse::GetHeader(const std::string& name) const {
    auto it = headers_.find(name);
    return (it != headers_.end()) ? it->second : "";
}

bool HttpResponse::HasHeader(const std::string& name) const {
    return headers_.find(name) != headers_.end();
}

void HttpResponse::RemoveHeader(const std::string& name) {
    headers_.erase(name);
}

void HttpResponse::SetCookie(const std::string& name, const std::string& value, 
                           const std::string& path, const std::string& domain,
                           bool secure, bool http_only) {
    HttpCookie cookie(name, value);
    cookie.path = path;
    cookie.domain = domain;
    cookie.secure = secure;
    cookie.http_only = http_only;
    cookies_.push_back(cookie);
}

void HttpResponse::SetBody(const std::string& body, const std::string& content_type) {
    body_ = body;
    SetContentType(content_type);
}

void HttpResponse::SetBody(std::string&& body, const std::string& content_type) {
    body_ = std::move(body);
    SetContentType(content_type);
}

void HttpResponse::SetBody(const std::vector<uint8_t>& body, const std::string& content_type) {
    body_.assign(body.begin(), body.end());
    SetContentType(content_type);
}

void HttpResponse::SetJsonBody(const nlohmann::json& json) {
    body_ = json.dump();
    SetContentType("application/json");
}

void HttpResponse::SetJsonBody(const std::string& json) {
    body_ = json;
    SetContentType("application/json");
}

nlohmann::json HttpResponse::GetJsonBody() const {
    try {
        return nlohmann::json::parse(body_);
    } catch (const nlohmann::json::exception&) {
        return nlohmann::json();
    }
}

void HttpResponse::SetHtmlBody(const std::string& html) {
    body_ = html;
    SetContentType("text/html");
}

void HttpResponse::SetFileBody(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (file.is_open()) {
        std::ostringstream oss;
        oss << file.rdbuf();
        body_ = oss.str();
        
        // Determine content type from file extension
        size_t dot_pos = file_path.find_last_of('.');
        if (dot_pos != std::string::npos) {
            std::string extension = file_path.substr(dot_pos);
            SetContentType(HttpUtils::GetMimeType(extension));
        } else {
            SetContentType("application/octet-stream");
        }
    }
}

size_t HttpResponse::GetContentLength() const {
    return body_.length();
}

std::string HttpResponse::GetContentType() const {
    return GetHeader("Content-Type");
}

void HttpResponse::SetContentType(const std::string& content_type) {
    SetHeader("Content-Type", content_type);
}

std::string HttpResponse::BuildStatusLine() const {
    std::string version_str = (version_ == HttpVersion::HTTP_1_0) ? "HTTP/1.0" : "HTTP/1.1";
    std::string reason = reason_phrase_.empty() ? HttpUtils::StatusCodeToString(status_code_) : reason_phrase_;
    return version_str + " " + std::to_string(static_cast<int>(status_code_)) + " " + reason;
}

std::string HttpResponse::BuildHeadersString() const {
    std::ostringstream oss;
    
    // Add standard headers
    for (const auto& [name, value] : headers_) {
        oss << name << ": " << value << "\r\n";
    }
    
    // Add cookies
    for (const auto& cookie : cookies_) {
        oss << "Set-Cookie: " << cookie.name << "=" << cookie.value;
        if (!cookie.path.empty()) {
            oss << "; Path=" << cookie.path;
        }
        if (!cookie.domain.empty()) {
            oss << "; Domain=" << cookie.domain;
        }
        if (cookie.secure) {
            oss << "; Secure";
        }
        if (cookie.http_only) {
            oss << "; HttpOnly";
        }
        if (!cookie.same_site.empty()) {
            oss << "; SameSite=" << cookie.same_site;
        }
        oss << "\r\n";
    }
    
    return oss.str();
}

std::string HttpResponse::ToString() const {
    std::ostringstream oss;
    oss << BuildStatusLine() << "\r\n";
    oss << BuildHeadersString();
    oss << "\r\n";
    if (!body_.empty()) {
        oss << body_;
    }
    return oss.str();
}

HttpResponse HttpResponse::FromBeastResponse(const boost::beast::http::response<boost::beast::http::string_body>& beast_resp) {
    HttpResponse response;
    
    // Convert status
    response.status_code_ = HttpUtils::BeastStatusToEnum(beast_resp.result());
    response.reason_phrase_ = beast_resp.reason();
    
    // Convert version
    response.version_ = (beast_resp.version() == 10) ? HttpVersion::HTTP_1_0 : HttpVersion::HTTP_1_1;
    
    // Convert headers
    for (const auto& field : beast_resp) {
        std::string name(field.name_string());
        std::string value(field.value());
        
        if (name == "Set-Cookie") {
            auto cookies = HttpUtils::ParseCookies(value);
            response.cookies_.insert(response.cookies_.end(), cookies.begin(), cookies.end());
        } else {
            response.headers_[name] = value;
        }
    }
    
    // Copy body
    response.body_ = beast_resp.body();
    
    return response;
}

// Static factory methods
HttpResponse HttpResponse::Ok(const std::string& body, const std::string& content_type) {
    HttpResponse response(HttpStatusCode::OK);
    if (!body.empty()) {
        response.SetBody(body, content_type);
    }
    return response;
}

HttpResponse HttpResponse::Created(const std::string& body, const std::string& location) {
    HttpResponse response(HttpStatusCode::CREATED);
    if (!body.empty()) {
        response.SetBody(body, "text/plain");
    }
    if (!location.empty()) {
        response.SetHeader("Location", location);
    }
    return response;
}

HttpResponse HttpResponse::NoContent() {
    return HttpResponse(HttpStatusCode::NO_CONTENT);
}

HttpResponse HttpResponse::BadRequest(const std::string& message) {
    HttpResponse response(HttpStatusCode::BAD_REQUEST);
    if (!message.empty()) {
        response.SetBody(message, "text/plain");
    }
    return response;
}

HttpResponse HttpResponse::Unauthorized(const std::string& message) {
    HttpResponse response(HttpStatusCode::UNAUTHORIZED);
    if (!message.empty()) {
        response.SetBody(message, "text/plain");
    }
    return response;
}

HttpResponse HttpResponse::Forbidden(const std::string& message) {
    HttpResponse response(HttpStatusCode::FORBIDDEN);
    if (!message.empty()) {
        response.SetBody(message, "text/plain");
    }
    return response;
}

HttpResponse HttpResponse::NotFound(const std::string& message) {
    HttpResponse response(HttpStatusCode::NOT_FOUND);
    if (!message.empty()) {
        response.SetBody(message, "text/plain");
    }
    return response;
}

HttpResponse HttpResponse::InternalServerError(const std::string& message) {
    HttpResponse response(HttpStatusCode::INTERNAL_SERVER_ERROR);
    if (!message.empty()) {
        response.SetBody(message, "text/plain");
    }
    return response;
}

HttpResponse HttpResponse::Json(const nlohmann::json& json, HttpStatusCode status) {
    HttpResponse response(status);
    response.SetJsonBody(json);
    return response;
}

HttpResponse HttpResponse::Html(const std::string& html, HttpStatusCode status) {
    HttpResponse response(status);
    response.SetHtmlBody(html);
    return response;
}

HttpResponse HttpResponse::Redirect(const std::string& location, HttpStatusCode status) {
    HttpResponse response(status);
    response.SetHeader("Location", location);
    return response;
}

} // namespace http
} // namespace network
} // namespace common