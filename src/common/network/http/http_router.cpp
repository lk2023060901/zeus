#include "common/network/http/http_router.h"
#include "common/network/network_logger.h"
#include <algorithm>
#include <sstream>
#include <regex>

namespace common {
namespace network {
namespace http {

// ===== RouteGroup Implementation =====

RouteGroup::RouteGroup(const std::string& prefix, HttpRouter& parent)
    : prefix_(prefix), parent_router_(parent) {
    
    // 标准化前缀
    full_prefix_ = prefix_;
    if (!full_prefix_.empty() && full_prefix_.back() != '/') {
        full_prefix_ += '/';
    }
}

RouteGroup& RouteGroup::Get(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::GET, path, std::move(handler), name);
}

RouteGroup& RouteGroup::Post(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::POST, path, std::move(handler), name);
}

RouteGroup& RouteGroup::Put(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::PUT, path, std::move(handler), name);
}

RouteGroup& RouteGroup::Delete(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::DELETE, path, std::move(handler), name);
}

RouteGroup& RouteGroup::Patch(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::PATCH, path, std::move(handler), name);
}

RouteGroup& RouteGroup::Head(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::HEAD, path, std::move(handler), name);
}

RouteGroup& RouteGroup::Options(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::OPTIONS, path, std::move(handler), name);
}

RouteGroup& RouteGroup::RegisterRoute(HttpMethod method, const std::string& path, HttpHandler handler, const std::string& name) {
    std::string full_path = full_prefix_ + path;
    if (full_path.length() > 1 && full_path.back() == '/') {
        full_path.pop_back(); // 移除末尾斜杠，除非是根路径
    }
    
    ::common::network::http::Route route;
    route.method = method;
    route.pattern = full_path;
    route.handler = std::move(handler);
    route.name = name;
    
    parent_router_.AddRoute(route, group_middlewares_);
    return *this;
}

RouteGroup& RouteGroup::Any(const std::string& path, HttpHandler handler, const std::string& name) {
    std::vector<HttpMethod> methods = {
        HttpMethod::GET, HttpMethod::POST, HttpMethod::PUT, 
        HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD, HttpMethod::OPTIONS
    };
    
    for (auto method : methods) {
        Route(method, path, handler, name + "_" + std::to_string(static_cast<int>(method)));
    }
    
    return *this;
}

RouteGroup& RouteGroup::Use(HttpMiddleware middleware) {
    group_middlewares_.push_back(std::move(middleware));
    return *this;
}

std::unique_ptr<RouteGroup> RouteGroup::Group(const std::string& prefix) {
    std::string new_prefix = full_prefix_ + prefix;
    return std::make_unique<RouteGroup>(new_prefix, parent_router_);
}

// ===== HttpRouter Implementation =====

HttpRouter::HttpRouter() {
    stats_.created_at = std::chrono::steady_clock::now();
}

HttpRouter::~HttpRouter() = default;

HttpRouter& HttpRouter::Get(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::GET, path, std::move(handler), name);
}

HttpRouter& HttpRouter::Post(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::POST, path, std::move(handler), name);
}

HttpRouter& HttpRouter::Put(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::PUT, path, std::move(handler), name);
}

HttpRouter& HttpRouter::Delete(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::DELETE, path, std::move(handler), name);
}

HttpRouter& HttpRouter::Patch(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::PATCH, path, std::move(handler), name);
}

HttpRouter& HttpRouter::Head(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::HEAD, path, std::move(handler), name);
}

HttpRouter& HttpRouter::Options(const std::string& path, HttpHandler handler, const std::string& name) {
    return RegisterRoute(HttpMethod::OPTIONS, path, std::move(handler), name);
}

HttpRouter& HttpRouter::RegisterRoute(HttpMethod method, const std::string& path, HttpHandler handler, const std::string& name) {
    ::common::network::http::Route route;
    route.method = method;
    route.pattern = path;
    route.handler = std::move(handler);
    route.name = name;
    
    AddRoute(route);
    return *this;
}

HttpRouter& HttpRouter::Any(const std::string& path, HttpHandler handler, const std::string& name) {
    std::vector<HttpMethod> methods = {
        HttpMethod::GET, HttpMethod::POST, HttpMethod::PUT, 
        HttpMethod::DELETE, HttpMethod::PATCH, HttpMethod::HEAD, HttpMethod::OPTIONS
    };
    
    for (auto method : methods) {
        Route(method, path, handler, name + "_" + std::to_string(static_cast<int>(method)));
    }
    
    return *this;
}

HttpRouter& HttpRouter::Use(HttpMiddleware middleware) {
    global_middlewares_.push_back(std::move(middleware));
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_middlewares++;
    }
    return *this;
}

HttpRouter& HttpRouter::Use(const std::string& path, HttpMiddleware middleware) {
    path_middlewares_[path].push_back(std::move(middleware));
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_middlewares++;
    }
    return *this;
}

std::unique_ptr<RouteGroup> HttpRouter::Group(const std::string& prefix) {
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_groups++;
    }
    return std::make_unique<RouteGroup>(prefix, *this);
}

RouteMatch HttpRouter::Match(HttpMethod method, const std::string& path) const {
    RouteMatch result;
    
    for (size_t i = 0; i < routes_.size(); ++i) {
        const auto& route = routes_[i];
        
        if (route.method != method) {
            continue;
        }
        
        RouteMatch single_match = MatchSingle(route, path);
        if (single_match.matched) {
            result = std::move(single_match);
            break;
        }
    }
    
    // 解析查询字符串
    auto query_pos = path.find('?');
    if (query_pos != std::string::npos && result.matched) {
        std::string query = path.substr(query_pos + 1);
        result.queries = ParseQueryString(query);
    }
    
    return result;
}

bool HttpRouter::HandleRequest(const HttpRequest& request, HttpResponse& response) {
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.requests_processed++;
    }
    
    try {
        // 匹配路由
        RouteMatch match = Match(request.GetMethod(), request.GetUrl().path);
        if (!match.matched) {
            Handle404(request, response);
            return false;
        }
        
        // 找到对应的路由
        const struct Route* matched_route = nullptr;
        for (const auto& route : routes_) {
            RouteMatch single_match = MatchSingle(route, request.GetUrl().path);
            if (single_match.matched && route.method == request.GetMethod()) {
                matched_route = &route;
                break;
            }
        }
        
        if (!matched_route) {
            Handle404(request, response);
            return false;
        }
        
        // 准备中间件链
        std::vector<HttpMiddleware> middlewares = global_middlewares_;
        
        // 添加路径特定中间件
        for (const auto& path_middleware : path_middlewares_) {
            if (request.GetUrl().path.find(path_middleware.first) == 0) {
                middlewares.insert(middlewares.end(), 
                                 path_middleware.second.begin(), 
                                 path_middleware.second.end());
            }
        }
        
        // 添加路由特定中间件
        middlewares.insert(middlewares.end(), 
                          matched_route->middlewares.begin(), 
                          matched_route->middlewares.end());
        
        // 执行中间件链，最后执行路由处理器
        ExecuteMiddlewares(request, response, middlewares, 0, [matched_route, &request, &response, match]() {
            // 将路由参数添加到请求中（这里需要扩展HttpRequest类来支持参数存储）
            // 暂时通过函数调用传递参数
            matched_route->handler(request, response, [](){});
        });
        
        return true;
        
    } catch (const std::exception& e) {
        Handle500(request, response, e);
        return true;
    }
}

const Route* HttpRouter::FindRoute(const std::string& name) const {
    auto it = named_routes_.find(name);
    if (it != named_routes_.end()) {
        return &routes_[it->second];
    }
    return nullptr;
}

std::string HttpRouter::GenerateUrl(const std::string& route_name, 
                                    const std::unordered_map<std::string, std::string>& params) const {
    const Route* route = FindRoute(route_name);
    if (!route) {
        return "";
    }
    
    std::string url = route->pattern;
    
    // 替换参数占位符
    for (const auto& param : params) {
        std::string placeholder = "{" + param.first + "}";
        auto pos = url.find(placeholder);
        if (pos != std::string::npos) {
            url.replace(pos, placeholder.length(), param.second);
        }
    }
    
    return url;
}

HttpRouter::RouterStats HttpRouter::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    RouterStats stats = stats_;
    stats.total_routes = routes_.size();
    
    // 统计各方法的路由数量
    stats.method_counts.clear();
    for (const auto& route : routes_) {
        std::string method_name = "unknown";
        switch (route.method) {
            case HttpMethod::GET: method_name = "GET"; break;
            case HttpMethod::POST: method_name = "POST"; break;
            case HttpMethod::PUT: method_name = "PUT"; break;
            case HttpMethod::DELETE: method_name = "DELETE"; break;
            case HttpMethod::PATCH: method_name = "PATCH"; break;
            case HttpMethod::HEAD: method_name = "HEAD"; break;
            case HttpMethod::OPTIONS: method_name = "OPTIONS"; break;
            default: method_name = "OTHER"; break;
        }
        stats.method_counts[method_name]++;
    }
    
    return stats;
}

void HttpRouter::Clear() {
    routes_.clear();
    global_middlewares_.clear();
    path_middlewares_.clear();
    named_routes_.clear();
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = RouterStats{};
    stats_.created_at = std::chrono::steady_clock::now();
}

void HttpRouter::AddRoute(const Route& route, const std::vector<HttpMiddleware>& group_middlewares) {
    Route new_route = route;
    
    // 编译路径模式
    auto compiled = CompilePattern(route.pattern);
    new_route.regex = std::move(compiled.first);
    new_route.param_names = std::move(compiled.second);
    
    // 添加组中间件
    new_route.middlewares.insert(new_route.middlewares.begin(),
                                 group_middlewares.begin(), group_middlewares.end());
    
    // 如果有名称，记录到命名路由索引
    if (!new_route.name.empty()) {
        named_routes_[new_route.name] = routes_.size();
    }
    
    routes_.push_back(std::move(new_route));
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_routes++;
}

RouteMatch HttpRouter::MatchSingle(const Route& route, const std::string& path) const {
    RouteMatch result;
    
    std::smatch match;
    if (std::regex_match(path, match, route.regex)) {
        result.matched = true;
        result.matched_pattern = route.pattern;
        result.matched_path = path;
        
        // 提取参数
        for (size_t i = 1; i < match.size() && (i - 1) < route.param_names.size(); ++i) {
            result.params[route.param_names[i - 1]] = match[i].str();
        }
    }
    
    return result;
}

std::unordered_map<std::string, std::string> HttpRouter::ParseQueryString(const std::string& query) const {
    std::unordered_map<std::string, std::string> result;
    
    std::stringstream ss(query);
    std::string pair;
    
    while (std::getline(ss, pair, '&')) {
        auto eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = pair.substr(0, eq_pos);
            std::string value = pair.substr(eq_pos + 1);
            result[key] = value;
        } else {
            result[pair] = "";
        }
    }
    
    return result;
}

std::pair<std::regex, std::vector<std::string>> HttpRouter::CompilePattern(const std::string& pattern) const {
    std::vector<std::string> param_names;
    std::string regex_pattern = PatternToRegex(pattern, param_names);
    
    std::regex::flag_type flags = std::regex::optimize;
    if (!case_sensitive_) {
        flags |= std::regex::icase;
    }
    
    return {std::regex(regex_pattern, flags), std::move(param_names)};
}

std::string HttpRouter::PatternToRegex(const std::string& pattern, std::vector<std::string>& param_names) const {
    std::string result = pattern;
    
    // 转义特殊字符
    std::regex special_chars(R"([\.^$|()[\]{}*+?\\])");
    result = std::regex_replace(result, special_chars, R"(\$&)");
    
    // 处理参数占位符 {param}
    std::regex param_regex(R"(\\\{([^}]+)\\\})");
    std::sregex_iterator iter(result.begin(), result.end(), param_regex);
    std::sregex_iterator end;
    
    size_t offset = 0;
    for (; iter != end; ++iter) {
        const std::smatch& match = *iter;
        param_names.push_back(match[1].str());
        
        size_t pos = match.position() + offset;
        size_t len = match.length();
        result.replace(pos, len, "([^/]+)");
        offset += 7 - len; // "([^/]+)" 有7个字符
    }
    
    // 处理通配符 *
    std::regex wildcard_regex(R"(\\\*)");
    result = std::regex_replace(result, wildcard_regex, "(.*)");
    
    // 严格斜杠匹配
    if (!strict_slash_) {
        if (result.back() == '/') {
            result += "?";
        } else {
            result += "/?";
        }
    }
    
    return "^" + result + "$";
}

void HttpRouter::ExecuteMiddlewares(const HttpRequest& request, HttpResponse& response, 
                                   const std::vector<HttpMiddleware>& middlewares, 
                                   size_t index, std::function<void()> final_handler) {
    if (index >= middlewares.size()) {
        final_handler();
        return;
    }
    
    auto next = [this, &request, &response, &middlewares, index, final_handler]() {
        ExecuteMiddlewares(request, response, middlewares, index + 1, final_handler);
    };
    
    try {
        middlewares[index](request, response, next);
    } catch (const std::exception& e) {
        Handle500(request, response, e);
    }
}

void HttpRouter::Handle404(const HttpRequest& request, HttpResponse& response) {
    response.SetStatusCode(HttpStatusCode::NOT_FOUND);
    response.SetHeader("Content-Type", "text/plain");
    response.SetBody("404 Not Found");
    
    NETWORK_LOG_DEBUG("HTTP 404: {} {}", 
                     static_cast<int>(request.GetMethod()), 
                     request.GetUrl().path);
}

void HttpRouter::Handle405(const HttpRequest& request, HttpResponse& response, const std::vector<HttpMethod>& allowed_methods) {
    response.SetStatusCode(HttpStatusCode::METHOD_NOT_ALLOWED);
    response.SetHeader("Content-Type", "text/plain");
    
    // 构建Allow头
    std::string allow_header;
    for (size_t i = 0; i < allowed_methods.size(); ++i) {
        if (i > 0) allow_header += ", ";
        
        switch (allowed_methods[i]) {
            case HttpMethod::GET: allow_header += "GET"; break;
            case HttpMethod::POST: allow_header += "POST"; break;
            case HttpMethod::PUT: allow_header += "PUT"; break;
            case HttpMethod::DELETE: allow_header += "DELETE"; break;
            case HttpMethod::PATCH: allow_header += "PATCH"; break;
            case HttpMethod::HEAD: allow_header += "HEAD"; break;
            case HttpMethod::OPTIONS: allow_header += "OPTIONS"; break;
            default: allow_header += "UNKNOWN"; break;
        }
    }
    
    response.SetHeader("Allow", allow_header);
    response.SetBody("405 Method Not Allowed");
    
    NETWORK_LOG_DEBUG("HTTP 405: {} {}", 
                     static_cast<int>(request.GetMethod()), 
                     request.GetUrl().path);
}

void HttpRouter::Handle500(const HttpRequest& request, HttpResponse& response, const std::exception& error) {
    response.SetStatusCode(HttpStatusCode::INTERNAL_SERVER_ERROR);
    response.SetHeader("Content-Type", "text/plain");
    response.SetBody("500 Internal Server Error");
    
    NETWORK_LOG_ERROR("HTTP 500: {} {} - Error: {}", 
                     static_cast<int>(request.GetMethod()), 
                     request.GetUrl().path,
                     error.what());
}

// ===== HttpRouterBuilder Implementation =====

std::unique_ptr<HttpRouter> HttpRouterBuilder::Build() {
    auto router = std::make_unique<HttpRouter>();
    
    // TODO: Add public setters for these configuration options
    // router->case_sensitive_ = case_sensitive_;
    // router->strict_slash_ = strict_slash_;
    // router->merge_params_ = merge_params_;
    
    for (auto& middleware : global_middlewares_) {
        router->Use(std::move(middleware));
    }
    
    return router;
}

// ===== PathMatcher Implementation =====

bool PathMatcher::WildcardMatch(const std::string& pattern, const std::string& path) {
    // Simplified pattern matching - TODO: implement full PatternToRegex
    return pattern == "*" || path.find(pattern) != std::string::npos;
}

std::string PathMatcher::GetFileExtension(const std::string& path) {
    auto pos = path.find_last_of('.');
    if (pos == std::string::npos || pos == path.length() - 1) {
        return "";
    }
    return path.substr(pos + 1);
}

std::string PathMatcher::NormalizePath(const std::string& path) {
    if (path.empty()) {
        return "/";
    }
    
    std::string result = path;
    
    // 确保以/开头
    if (result.front() != '/') {
        result = "/" + result;
    }
    
    // 移除重复的斜杠
    std::regex multiple_slash(R"(/+)");
    result = std::regex_replace(result, multiple_slash, "/");
    
    // 处理 . 和 ..
    std::vector<std::string> segments = SplitPath(result);
    std::vector<std::string> normalized_segments;
    
    for (const auto& segment : segments) {
        if (segment == "." || segment.empty()) {
            continue;
        } else if (segment == "..") {
            if (!normalized_segments.empty()) {
                normalized_segments.pop_back();
            }
        } else {
            normalized_segments.push_back(segment);
        }
    }
    
    if (normalized_segments.empty()) {
        return "/";
    }
    
    return "/" + JoinPaths(normalized_segments);
}

std::string PathMatcher::JoinPaths(const std::vector<std::string>& segments) {
    if (segments.empty()) {
        return "";
    }
    
    std::string result;
    for (size_t i = 0; i < segments.size(); ++i) {
        if (i > 0) {
            result += "/";
        }
        result += segments[i];
    }
    
    return result;
}

std::vector<std::string> PathMatcher::SplitPath(const std::string& path) {
    std::vector<std::string> segments;
    std::stringstream ss(path);
    std::string segment;
    
    while (std::getline(ss, segment, '/')) {
        if (!segment.empty()) {
            segments.push_back(segment);
        }
    }
    
    return segments;
}

bool PathMatcher::IsAbsolutePath(const std::string& path) {
    return !path.empty() && path.front() == '/';
}

// This function should be in HttpRouter class, not PathMatcher
// Removing this implementation to fix compilation
/*
std::string PathMatcher::PatternToRegex(const std::string& pattern) {
    std::string result = pattern;
    
    // 转义特殊字符
    std::regex special_chars(R"([\.^$|()[\]{}+?\\])");
    result = std::regex_replace(result, special_chars, R"(\$&)");
    
    // 替换通配符
    std::regex star(R"(\*)");
    result = std::regex_replace(result, star, ".*");
    
    std::regex question(R"(\?)");
    result = std::regex_replace(result, question, ".");
    
    return "^" + result + "$";
}*/

} // namespace http
} // namespace network
} // namespace common