#pragma once

#include "http_common.h"
#include "http_message.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <regex>
#include <memory>

namespace common {
namespace network {
namespace http {

// Forward declarations
class HttpRouter;
class RouteGroup;

/**
 * @brief 路由条目
 */
struct Route {
    HttpMethod method;                          // HTTP方法
    std::string pattern;                        // 路径模式
    std::regex regex;                          // 编译的正则表达式
    std::vector<std::string> param_names;      // 参数名称列表
    HttpHandler handler;                       // 请求处理器
    std::vector<HttpMiddleware> middlewares;   // 路由特定中间件
    std::string name;                          // 路由名称（可选）
    
    Route() = default;
    Route(HttpMethod m, const std::string& p, HttpHandler h, const std::string& n = "")
        : method(m), pattern(p), handler(std::move(h)), name(n) {}
};

/**
 * @brief 路由组，用于组织相关路由
 */
class RouteGroup {
public:
    /**
     * @brief 构造函数
     * @param prefix URL前缀
     * @param parent 父路由器引用
     */
    RouteGroup(const std::string& prefix, HttpRouter& parent);
    
    /**
     * @brief 析构函数
     */
    ~RouteGroup() = default;
    
    // HTTP方法路由注册
    RouteGroup& Get(const std::string& path, HttpHandler handler, const std::string& name = "");
    RouteGroup& Post(const std::string& path, HttpHandler handler, const std::string& name = "");
    RouteGroup& Put(const std::string& path, HttpHandler handler, const std::string& name = "");
    RouteGroup& Delete(const std::string& path, HttpHandler handler, const std::string& name = "");
    RouteGroup& Patch(const std::string& path, HttpHandler handler, const std::string& name = "");
    RouteGroup& Head(const std::string& path, HttpHandler handler, const std::string& name = "");
    RouteGroup& Options(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册任意HTTP方法路由
     */
    RouteGroup& RegisterRoute(HttpMethod method, const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册匹配所有HTTP方法的路由
     */
    RouteGroup& Any(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 添加中间件到路由组
     */
    RouteGroup& Use(HttpMiddleware middleware);
    
    /**
     * @brief 创建子路由组
     */
    std::unique_ptr<RouteGroup> Group(const std::string& prefix);
    
    /**
     * @brief 获取完整前缀（包括父组前缀）
     */
    std::string GetFullPrefix() const { return full_prefix_; }

private:
    std::string prefix_;
    std::string full_prefix_;
    HttpRouter& parent_router_;
    std::vector<HttpMiddleware> group_middlewares_;
};

/**
 * @brief HTTP路由器核心类
 */
class HttpRouter {
public:
    /**
     * @brief 构造函数
     */
    HttpRouter();
    
    /**
     * @brief 析构函数
     */
    ~HttpRouter();
    
    // 路由注册方法
    
    /**
     * @brief 注册GET路由
     */
    HttpRouter& Get(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册POST路由
     */
    HttpRouter& Post(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册PUT路由
     */
    HttpRouter& Put(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册DELETE路由
     */
    HttpRouter& Delete(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册PATCH路由
     */
    HttpRouter& Patch(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册HEAD路由
     */
    HttpRouter& Head(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册OPTIONS路由
     */
    HttpRouter& Options(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册任意HTTP方法路由
     */
    HttpRouter& RegisterRoute(HttpMethod method, const std::string& path, HttpHandler handler, const std::string& name = "");
    
    /**
     * @brief 注册匹配所有HTTP方法的路由
     */
    HttpRouter& Any(const std::string& path, HttpHandler handler, const std::string& name = "");
    
    // 中间件管理
    
    /**
     * @brief 添加全局中间件
     */
    HttpRouter& Use(HttpMiddleware middleware);
    
    /**
     * @brief 为特定路径添加中间件
     */
    HttpRouter& Use(const std::string& path, HttpMiddleware middleware);
    
    // 路由组管理
    
    /**
     * @brief 创建路由组
     */
    std::unique_ptr<RouteGroup> Group(const std::string& prefix);
    
    // 路由匹配和处理
    
    /**
     * @brief 匹配路由
     * @param method HTTP方法
     * @param path 请求路径
     * @return 匹配结果
     */
    RouteMatch Match(HttpMethod method, const std::string& path) const;
    
    /**
     * @brief 处理HTTP请求
     * @param request HTTP请求
     * @param response HTTP响应
     * @return 是否找到匹配的路由
     */
    bool HandleRequest(const HttpRequest& request, HttpResponse& response);
    
    // 路由信息查询
    
    /**
     * @brief 根据名称查找路由
     */
    const struct Route* FindRoute(const std::string& name) const;
    
    /**
     * @brief 生成URL（根据路由名称和参数）
     */
    std::string GenerateUrl(const std::string& route_name, 
                           const std::unordered_map<std::string, std::string>& params = {}) const;
    
    /**
     * @brief 获取所有路由
     */
    const std::vector<struct Route>& GetRoutes() const { return routes_; }
    
    /**
     * @brief 清空所有路由
     */
    void Clear();
    
    /**
     * @brief 获取路由统计信息
     */
    struct RouterStats {
        size_t total_routes = 0;
        size_t total_middlewares = 0;
        size_t total_groups = 0;
        std::unordered_map<std::string, size_t> method_counts;
        size_t requests_processed = 0;
        std::chrono::steady_clock::time_point created_at;
    };
    
    RouterStats GetStats() const;

    // 内部接口（供RouteGroup使用）
    void AddRoute(const struct Route& route, const std::vector<HttpMiddleware>& group_middlewares = {});

private:
    // 路由匹配辅助方法
    RouteMatch MatchSingle(const struct Route& route, const std::string& path) const;
    std::unordered_map<std::string, std::string> ParseQueryString(const std::string& query) const;
    
    // 路径模式编译
    std::pair<std::regex, std::vector<std::string>> CompilePattern(const std::string& pattern) const;
    std::string PatternToRegex(const std::string& pattern, std::vector<std::string>& param_names) const;
    
    // 中间件执行
    void ExecuteMiddlewares(const HttpRequest& request, HttpResponse& response, 
                           const std::vector<HttpMiddleware>& middlewares, 
                           size_t index, std::function<void()> final_handler);
    
    // 内建错误处理
    void Handle404(const HttpRequest& request, HttpResponse& response);
    void Handle405(const HttpRequest& request, HttpResponse& response, const std::vector<HttpMethod>& allowed_methods);
    void Handle500(const HttpRequest& request, HttpResponse& response, const std::exception& error);
    
    std::vector<struct Route> routes_;                             // 所有路由
    std::vector<HttpMiddleware> global_middlewares_;               // 全局中间件
    std::unordered_map<std::string, std::vector<HttpMiddleware>> path_middlewares_; // 路径中间件
    std::unordered_map<std::string, size_t> named_routes_;        // 命名路由索引
    
    // 统计信息
    mutable std::mutex stats_mutex_;
    RouterStats stats_;
    
    // 配置选项
    bool case_sensitive_ = false;                                  // 路径大小写敏感
    bool strict_slash_ = false;                                   // 严格斜杠匹配
    // bool merge_params_ = true;  // Temporarily disabled - 合并查询参数和路径参数
};

/**
 * @brief 路由器建造者模式
 */
class HttpRouterBuilder {
public:
    HttpRouterBuilder() = default;
    
    /**
     * @brief 设置大小写敏感
     */
    HttpRouterBuilder& CaseSensitive(bool sensitive = true) {
        case_sensitive_ = sensitive;
        return *this;
    }
    
    /**
     * @brief 设置严格斜杠匹配
     */
    HttpRouterBuilder& StrictSlash(bool strict = true) {
        strict_slash_ = strict;
        return *this;
    }
    
    /**
     * @brief 设置参数合并
     */
    HttpRouterBuilder& MergeParams(bool merge = true) {
        merge_params_ = merge;
        return *this;
    }
    
    /**
     * @brief 添加全局中间件
     */
    HttpRouterBuilder& Use(HttpMiddleware middleware) {
        global_middlewares_.push_back(std::move(middleware));
        return *this;
    }
    
    /**
     * @brief 构建路由器
     */
    std::unique_ptr<HttpRouter> Build();

private:
    bool case_sensitive_ = false;
    bool strict_slash_ = false;
    bool merge_params_ = true;
    std::vector<HttpMiddleware> global_middlewares_;
};

// 常用路径模式匹配工具

/**
 * @brief 路径匹配工具类
 */
class PathMatcher {
public:
    /**
     * @brief 检查路径是否匹配通配符模式
     * @param pattern 模式（支持 * 和 ? 通配符）
     * @param path 待匹配路径
     * @return 是否匹配
     */
    static bool WildcardMatch(const std::string& pattern, const std::string& path);
    
    /**
     * @brief 提取路径中的文件扩展名
     */
    static std::string GetFileExtension(const std::string& path);
    
    /**
     * @brief 标准化路径（移除重复斜杠等）
     */
    static std::string NormalizePath(const std::string& path);
    
    /**
     * @brief 连接路径片段
     */
    static std::string JoinPaths(const std::vector<std::string>& segments);
    
    /**
     * @brief 分割路径为片段
     */
    static std::vector<std::string> SplitPath(const std::string& path);
    
    /**
     * @brief 检查路径是否为绝对路径
     */
    static bool IsAbsolutePath(const std::string& path);
};

} // namespace http
} // namespace network
} // namespace common