#pragma once

#include "application_types.h"
#include "common/network/zeus_network.h"
#include <memory>
#include <boost/asio.hpp>

namespace core {
namespace app {

/**
 * @brief 服务工厂类，用于创建各种网络服务
 */
class ServiceFactory {
public:
    /**
     * @brief 构造函数
     * @param executor Boost.ASIO执行器
     */
    explicit ServiceFactory(boost::asio::any_io_executor executor);
    
    /**
     * @brief 析构函数
     */
    ~ServiceFactory() = default;
    
    // 禁止拷贝和赋值
    ServiceFactory(const ServiceFactory&) = delete;
    ServiceFactory& operator=(const ServiceFactory&) = delete;
    
    /**
     * @brief 创建TCP服务器
     * @param config 监听器配置
     * @param options 服务选项
     * @return TCP服务器实例
     */
    std::unique_ptr<Service> CreateTcpAcceptor(const ListenerConfig& config, const TcpServiceOptions& options);
    
    /**
     * @brief 创建HTTP服务器
     * @param config 监听器配置
     * @param options 服务选项
     * @return HTTP服务器实例
     */
    std::unique_ptr<Service> CreateHttpServer(const ListenerConfig& config, const HttpServiceOptions& options);
    
    /**
     * @brief 创建HTTPS服务器
     * @param config 监听器配置
     * @param options 服务选项
     * @return HTTPS服务器实例
     */
    std::unique_ptr<Service> CreateHttpsServer(const ListenerConfig& config, const HttpServiceOptions& options);
    
    /**
     * @brief 创建KCP服务器
     * @param config 监听器配置
     * @param options 服务选项
     * @return KCP服务器实例
     */
    std::unique_ptr<Service> CreateKcpAcceptor(const ListenerConfig& config, const KcpServiceOptions& options);
    
    /**
     * @brief 创建TCP客户端连接
     * @param config 连接器配置
     * @param options 服务选项
     * @return TCP客户端实例
     */
    std::unique_ptr<Service> CreateTcpClient(const ConnectorConfig& config, const TcpServiceOptions& options);
    
    /**
     * @brief 创建HTTP客户端
     * @param config 连接器配置
     * @param options 服务选项
     * @return HTTP客户端实例
     */
    std::unique_ptr<Service> CreateHttpClient(const ConnectorConfig& config, const HttpServiceOptions& options);
    
    /**
     * @brief 创建HTTPS客户端
     * @param config 连接器配置
     * @param options 服务选项
     * @return HTTPS客户端实例
     */
    std::unique_ptr<Service> CreateHttpsClient(const ConnectorConfig& config, const HttpServiceOptions& options);
    
    /**
     * @brief 创建KCP客户端连接
     * @param config 连接器配置
     * @param options 服务选项
     * @return KCP客户端实例
     */
    std::unique_ptr<Service> CreateKcpClient(const ConnectorConfig& config, const KcpServiceOptions& options);
    
    /**
     * @brief 根据配置自动创建服务
     * @param config 监听器配置
     * @param tcp_options TCP服务选项（可选）
     * @param http_options HTTP服务选项（可选）
     * @param kcp_options KCP服务选项（可选）
     * @return 创建的服务实例
     */
    std::unique_ptr<Service> CreateListener(
        const ListenerConfig& config,
        const std::optional<TcpServiceOptions>& tcp_options = std::nullopt,
        const std::optional<HttpServiceOptions>& http_options = std::nullopt,
        const std::optional<KcpServiceOptions>& kcp_options = std::nullopt
    );
    
    /**
     * @brief 根据配置自动创建连接器
     * @param config 连接器配置
     * @param tcp_options TCP服务选项（可选）
     * @param http_options HTTP服务选项（可选）
     * @param kcp_options KCP服务选项（可选）
     * @return 创建的服务实例
     */
    std::unique_ptr<Service> CreateConnector(
        const ConnectorConfig& config,
        const std::optional<TcpServiceOptions>& tcp_options = std::nullopt,
        const std::optional<HttpServiceOptions>& http_options = std::nullopt,
        const std::optional<KcpServiceOptions>& kcp_options = std::nullopt
    );
    
    /**
     * @brief 设置执行器
     * @param executor 新的执行器
     */
    void SetExecutor(boost::asio::any_io_executor executor) { executor_ = executor; }
    
    /**
     * @brief 获取执行器
     */
    boost::asio::any_io_executor GetExecutor() const { return executor_; }
    
    /**
     * @brief 创建TCP服务器
     */
    std::unique_ptr<Service> CreateTcpServer(const ListenerConfig& config, const TcpServiceOptions& options);
    
    /**
     * @brief 创建KCP服务器
     */
    std::unique_ptr<Service> CreateKcpServer(const ListenerConfig& config, const KcpServiceOptions& options);

private:
    // 工具方法
    common::network::http::HttpServerConfig CreateHttpServerConfig(const ListenerConfig& config);
    std::tuple<uint16_t, std::string> CreateTcpAcceptorConfig(const ListenerConfig& config);
    std::tuple<uint16_t, std::string, common::network::KcpConnector::KcpConfig> CreateKcpAcceptorConfig(const ListenerConfig& config);
    
    boost::asio::ssl::context CreateSSLContext(const SSLConfig& ssl_config, bool is_server);
    
    boost::asio::any_io_executor executor_;
};

// 服务适配器类，将Zeus网络组件适配为Application Service接口

/**
 * @brief TCP服务器适配器
 */
class TcpAcceptorAdapter : public Service {
public:
    TcpAcceptorAdapter(const std::string& name, std::shared_ptr<common::network::TcpAcceptor> server);
    
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;
    const std::string& GetName() const override;
    ServiceType GetType() const override;

private:
    std::string name_;
    std::shared_ptr<common::network::TcpAcceptor> server_;
    std::atomic<bool> running_{false};
};

/**
 * @brief HTTP服务器适配器
 */
class HttpServerAdapter : public Service {
public:
    HttpServerAdapter(const std::string& name, std::unique_ptr<common::network::http::HttpServer> server, bool is_https = false);
    
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;
    const std::string& GetName() const override;
    ServiceType GetType() const override;

private:
    std::string name_;
    std::unique_ptr<common::network::http::HttpServer> server_;
    bool is_https_;
    std::atomic<bool> running_{false};
};

/**
 * @brief KCP服务器适配器
 */
class KcpAcceptorAdapter : public Service {
public:
    KcpAcceptorAdapter(const std::string& name, std::shared_ptr<common::network::KcpAcceptor> server);
    
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;
    const std::string& GetName() const override;
    ServiceType GetType() const override;

private:
    std::string name_;
    std::shared_ptr<common::network::KcpAcceptor> server_;
    std::atomic<bool> running_{false};
};

/**
 * @brief TCP客户端适配器
 */
class TcpClientAdapter : public Service {
public:
    TcpClientAdapter(const std::string& name, std::shared_ptr<common::network::TcpConnector> connection, const std::vector<std::string>& targets);
    
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;
    const std::string& GetName() const override;
    ServiceType GetType() const override;

private:
    std::string name_;
    std::shared_ptr<common::network::TcpConnector> connection_;
    std::vector<std::string> targets_;
    std::atomic<bool> running_{false};
    size_t current_target_index_{0};
};

/**
 * @brief HTTP客户端适配器
 */
class HttpClientAdapter : public Service {
public:
    HttpClientAdapter(const std::string& name, std::unique_ptr<common::network::http::HttpClient> client, bool is_https = false);
    
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;
    const std::string& GetName() const override;
    ServiceType GetType() const override;

private:
    std::string name_;
    std::unique_ptr<common::network::http::HttpClient> client_;
    bool is_https_;
    std::atomic<bool> running_{false};
};

/**
 * @brief KCP客户端适配器
 */
class KcpClientAdapter : public Service {
public:
    KcpClientAdapter(const std::string& name, std::shared_ptr<common::network::KcpConnector> connection, const std::vector<std::string>& targets);
    
    bool Start() override;
    void Stop() override;
    bool IsRunning() const override;
    const std::string& GetName() const override;
    ServiceType GetType() const override;

private:
    std::string name_;
    std::shared_ptr<common::network::KcpConnector> connection_;
    std::vector<std::string> targets_;
    std::atomic<bool> running_{false};
    size_t current_target_index_{0};
};

} // namespace app
} // namespace core