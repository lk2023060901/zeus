/**
 * @file business_scenarios_example.cpp
 * @brief 实际业务场景结构化日志示例
 * 
 * 这个示例演示了在真实业务场景中如何使用结构化日志：
 * - Web服务API日志记录
 * - 用户认证和授权日志
 * - 数据库操作日志
 * - 支付处理日志
 * - 错误处理和监控日志
 * - 性能监控日志
 */

#include "common/spdlog/zeus_structured_log.h"
#include <iostream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <random>

using namespace common::spdlog::structured;

/**
 * @brief 模拟用户认证场景
 */
void simulate_user_authentication() {
    std::cout << "\n=== 用户认证场景 ===" << std::endl;
    
    auto auth_logger = ZEUS_GET_STRUCTURED_LOGGER("authentication");
    
    // 1. 成功登录
    auth_logger->info(
        business_fields::event_type("user_login_attempt"),
        business_fields::user_id(12345),
        FIELD("username", "john_doe"),
        business_fields::ip_address("192.168.1.100"),
        FIELD("user_agent", "Mozilla/5.0 Chrome/91.0"),
        FIELD("login_method", "password"),
        FIELD("success", true),
        business_fields::response_time_ms(156.78),
        fields::timestamp("login_time")
    );
    
    // 2. 失败登录 - 错误密码
    auth_logger->warn(
        business_fields::event_type("user_login_failed"),
        FIELD("username", "jane_smith"),
        business_fields::ip_address("10.0.0.15"),
        business_fields::error_code("AUTH_001"),
        business_fields::error_message("Invalid credentials"),
        FIELD("attempt_count", 3),
        FIELD("account_locked", false),
        fields::timestamp()
    );
    
    // 3. 账户被锁定
    auth_logger->error(
        business_fields::event_type("account_locked"),
        FIELD("username", "suspicious_user"),
        business_fields::ip_address("203.0.113.42"),
        FIELD("failed_attempts", 5),
        FIELD("lock_duration_minutes", 30),
        business_fields::error_code("AUTH_002"),
        business_fields::error_message("Account locked due to multiple failed attempts")
    );
    
    // 4. OAuth认证
    auth_logger->info(
        business_fields::event_type("oauth_login"),
        business_fields::user_id(67890),
        FIELD("oauth_provider", "google"),
        FIELD("oauth_client_id", "client_123abc"),
        FIELD("scope", "profile email"),
        FIELD("success", true),
        business_fields::response_time_ms(89.23)
    );
}

/**
 * @brief 模拟Web API请求处理
 */
void simulate_web_api_requests() {
    std::cout << "\n=== Web API请求处理场景 ===" << std::endl;
    
    auto api_logger = ZEUS_GET_STRUCTURED_LOGGER("web_api");
    
    // 1. 用户资料查询API
    patterns::http_access(api_logger, 
        "GET", "/api/v1/users/12345", 200, 45.67, 
        "Zeus-Mobile-App/2.1.0", "192.168.1.50");
    
    api_logger->info(
        business_fields::event_type("api_request"),
        business_fields::request_id("req_abc123def456"),
        business_fields::user_id(12345),
        business_fields::http_method("GET"),
        business_fields::http_path("/api/v1/users/12345"),
        business_fields::http_status(200),
        business_fields::response_time_ms(45.67),
        business_fields::request_size(0),
        business_fields::response_size(1024),
        FIELD("cache_hit", true),
        FIELD("database_queries", 0)
    );
    
    // 2. 创建新订单API
    api_logger->info(
        business_fields::event_type("order_creation"),
        business_fields::request_id("req_order_789xyz"),
        business_fields::user_id(12345),
        business_fields::http_method("POST"),
        business_fields::http_path("/api/v1/orders"),
        FIELD("order_id", "ORDER_2024_001"),
        FIELD("product_count", 3),
        FIELD("total_amount", 299.99),
        FIELD("currency", "USD"),
        FIELD("payment_method", "credit_card"),
        business_fields::response_time_ms(234.56),
        FIELD("validation_time_ms", 23.45),
        FIELD("database_save_time_ms", 89.12)
    );
    
    // 3. API限流触发
    api_logger->warn(
        business_fields::event_type("rate_limit_exceeded"),
        business_fields::request_id("req_limited_999"),
        business_fields::user_id(54321),
        business_fields::ip_address("203.0.113.15"),
        business_fields::http_method("POST"),
        business_fields::http_path("/api/v1/orders"),
        business_fields::http_status(429),
        FIELD("rate_limit_type", "per_user"),
        FIELD("limit_requests_per_minute", 100),
        FIELD("current_requests", 102),
        FIELD("reset_time_seconds", 45)
    );
    
    // 4. API内部错误
    api_logger->error(
        business_fields::event_type("api_internal_error"),
        business_fields::request_id("req_error_888"),
        business_fields::user_id(98765),
        business_fields::http_method("PUT"),
        business_fields::http_path("/api/v1/users/98765/profile"),
        business_fields::http_status(500),
        business_fields::error_code("DB_CONNECTION_FAILED"),
        business_fields::error_message("Failed to connect to primary database"),
        FIELD("retry_attempt", 3),
        FIELD("fallback_used", false),
        business_fields::response_time_ms(5000.0)
    );
}

/**
 * @brief 模拟支付处理场景
 */
void simulate_payment_processing() {
    std::cout << "\n=== 支付处理场景 ===" << std::endl;
    
    auto payment_logger = ZEUS_GET_STRUCTURED_LOGGER("payments");
    
    // 1. 支付开始
    payment_logger->info(
        business_fields::event_type("payment_initiated"),
        FIELD("payment_id", "PAY_2024_123456"),
        FIELD("order_id", "ORDER_2024_001"),
        business_fields::user_id(12345),
        FIELD("amount", 299.99),
        FIELD("currency", "USD"),
        FIELD("payment_method", "credit_card"),
        FIELD("card_last_four", "1234"),
        FIELD("payment_gateway", "stripe"),
        fields::timestamp("initiated_at")
    );
    
    // 模拟处理时间
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 2. 支付验证
    payment_logger->debug(
        business_fields::event_type("payment_validation"),
        FIELD("payment_id", "PAY_2024_123456"),
        FIELD("fraud_check_result", "passed"),
        FIELD("risk_score", 0.15),
        FIELD("3ds_required", false),
        FIELD("validation_time_ms", 89.45)
    );
    
    // 3. 支付成功
    payment_logger->info(
        business_fields::event_type("payment_completed"),
        FIELD("payment_id", "PAY_2024_123456"),
        FIELD("transaction_id", "txn_stripe_789abc"),
        FIELD("gateway_response_code", "approved"),
        FIELD("processing_fee", 9.27),
        business_fields::processing_time_ms(156.78),
        FIELD("success", true),
        fields::timestamp("completed_at")
    );
    
    // 4. 支付失败场景
    payment_logger->error(
        business_fields::event_type("payment_failed"),
        FIELD("payment_id", "PAY_2024_789012"),
        FIELD("order_id", "ORDER_2024_002"),
        business_fields::user_id(54321),
        FIELD("amount", 149.99),
        business_fields::error_code("INSUFFICIENT_FUNDS"),
        business_fields::error_message("Card declined - insufficient funds"),
        FIELD("gateway_error_code", "card_declined"),
        FIELD("retry_eligible", true),
        business_fields::processing_time_ms(234.56)
    );
}

/**
 * @brief 模拟数据库操作场景
 */
void simulate_database_operations() {
    std::cout << "\n=== 数据库操作场景 ===" << std::endl;
    
    auto db_logger = ZEUS_GET_STRUCTURED_LOGGER("database");
    
    // 1. 查询操作
    db_logger->debug(
        business_fields::event_type("database_query"),
        business_fields::operation("SELECT"),
        FIELD("table", "users"),
        FIELD("query_type", "primary_key_lookup"),
        FIELD("user_id", 12345),
        FIELD("execution_time_ms", 12.34),
        FIELD("rows_examined", 1),
        FIELD("rows_returned", 1),
        FIELD("index_used", "PRIMARY"),
        FIELD("connection_pool_size", 10),
        FIELD("connection_id", "conn_15")
    );
    
    // 2. 复杂查询
    db_logger->info(
        business_fields::event_type("database_query"),
        business_fields::operation("SELECT with JOIN"),
        FIELD("tables", "orders, users, products"),
        FIELD("query_type", "analytics"),
        FIELD("execution_time_ms", 456.78),
        FIELD("rows_examined", 50000),
        FIELD("rows_returned", 1500),
        FIELD("memory_used_mb", 12.5),
        FIELD("temporary_table_created", true),
        FIELD("query_hash", "abc123def456")
    );
    
    // 3. 写入操作
    db_logger->info(
        business_fields::event_type("database_insert"),
        business_fields::operation("INSERT"),
        FIELD("table", "orders"),
        FIELD("order_id", "ORDER_2024_001"),
        FIELD("execution_time_ms", 23.45),
        FIELD("affected_rows", 1),
        FIELD("auto_increment_id", 987654),
        FIELD("binlog_position", "mysql-bin.000123:456789")
    );
    
    // 4. 数据库连接失败
    db_logger->error(
        business_fields::event_type("database_connection_failed"),
        business_fields::error_code("DB_CONN_001"),
        business_fields::error_message("Connection timeout to primary database"),
        FIELD("database_host", "db-primary.internal"),
        FIELD("database_port", 3306),
        FIELD("timeout_seconds", 30),
        FIELD("retry_attempt", 3),
        FIELD("fallback_available", true),
        FIELD("connection_pool_exhausted", false)
    );
    
    // 5. 慢查询警告
    db_logger->warn(
        business_fields::event_type("slow_query_detected"),
        business_fields::operation("SELECT"),
        FIELD("execution_time_ms", 2340.56),
        FIELD("slow_query_threshold_ms", 1000),
        FIELD("query_hash", "slow_query_789xyz"),
        FIELD("table", "user_activities"),
        FIELD("rows_examined", 1000000),
        FIELD("optimization_suggested", "add_index_on_created_at")
    );
}

/**
 * @brief 模拟系统监控场景
 */
void simulate_system_monitoring() {
    std::cout << "\n=== 系统监控场景 ===" << std::endl;
    
    auto monitor_logger = ZEUS_GET_STRUCTURED_LOGGER("monitoring");
    
    // 随机数生成器用于模拟指标
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> cpu_dist(20.0, 90.0);
    std::uniform_real_distribution<double> memory_dist(40.0, 85.0);
    std::uniform_int_distribution<int> conn_dist(50, 200);
    
    // 1. 系统性能指标
    patterns::performance_metric(monitor_logger, 
        "system_health_check", 
        5.23, 
        cpu_dist(gen), 
        memory_dist(gen) * 10  // 转换为MB
    );
    
    monitor_logger->info(
        business_fields::event_type("system_metrics"),
        business_fields::cpu_usage(cpu_dist(gen)),
        business_fields::memory_usage_mb(memory_dist(gen) * 10),
        FIELD("disk_usage_percent", 67.5),
        FIELD("network_io_mbps", 125.6),
        FIELD("load_average_1min", 2.34),
        FIELD("load_average_5min", 1.98),
        business_fields::active_connections(conn_dist(gen)),
        FIELD("uptime_seconds", 86400 * 7), // 7 days
        fields::timestamp("collected_at")
    );
    
    // 2. 应用程序指标
    monitor_logger->info(
        business_fields::event_type("application_metrics"),
        FIELD("requests_per_second", 150.5),
        FIELD("error_rate_percent", 0.05),
        FIELD("avg_response_time_ms", 89.23),
        FIELD("p95_response_time_ms", 234.56),
        FIELD("p99_response_time_ms", 567.89),
        FIELD("active_sessions", conn_dist(gen)),
        FIELD("cache_hit_rate", 0.87),
        FIELD("queue_depth", 12)
    );
    
    // 3. 告警触发
    monitor_logger->critical(
        business_fields::event_type("system_alert"),
        FIELD("alert_type", "HIGH_CPU_USAGE"),
        FIELD("alert_severity", "critical"),
        business_fields::cpu_usage(95.8),
        FIELD("threshold_exceeded", 90.0),
        FIELD("duration_seconds", 300),
        FIELD("affected_services", "web-api, background-jobs"),
        FIELD("auto_scaling_triggered", true),
        FIELD("notification_sent", true)
    );
    
    // 4. 磁盘空间警告
    monitor_logger->warn(
        business_fields::event_type("disk_space_warning"),
        FIELD("mount_point", "/var/log"),
        FIELD("disk_usage_percent", 82.5),
        FIELD("available_gb", 15.6),
        FIELD("warning_threshold", 80.0),
        FIELD("estimated_full_in_hours", 48),
        FIELD("cleanup_job_scheduled", true)
    );
}

/**
 * @brief 模拟微服务间通信场景
 */
void simulate_microservice_communication() {
    std::cout << "\n=== 微服务间通信场景 ===" << std::endl;
    
    auto service_logger = ZEUS_GET_STRUCTURED_LOGGER("microservices");
    
    // 1. 服务间HTTP调用
    service_logger->info(
        business_fields::event_type("service_call_outbound"),
        FIELD("caller_service", "order-service"),
        FIELD("target_service", "inventory-service"),
        business_fields::http_method("POST"),
        FIELD("endpoint", "/api/v1/inventory/reserve"),
        business_fields::request_id("req_microservice_123"),
        business_fields::correlation_id("corr_order_456"),
        FIELD("payload_size", 256),
        business_fields::response_time_ms(78.9),
        business_fields::http_status(200),
        FIELD("circuit_breaker_state", "closed")
    );
    
    // 2. 消息队列发布
    service_logger->info(
        business_fields::event_type("message_published"),
        FIELD("publisher_service", "order-service"),
        FIELD("queue_name", "order.created"),
        FIELD("message_id", "msg_789abc123def"),
        business_fields::correlation_id("corr_order_456"),
        FIELD("message_size", 512),
        FIELD("routing_key", "order.created.v1"),
        FIELD("exchange", "orders"),
        FIELD("persistent", true),
        FIELD("publish_time_ms", 12.34)
    );
    
    // 3. 消息队列消费
    service_logger->info(
        business_fields::event_type("message_consumed"),
        FIELD("consumer_service", "notification-service"),
        FIELD("queue_name", "order.created"),
        FIELD("message_id", "msg_789abc123def"),
        business_fields::correlation_id("corr_order_456"),
        FIELD("processing_time_ms", 45.67),
        FIELD("retry_count", 0),
        FIELD("success", true),
        FIELD("ack_sent", true)
    );
    
    // 4. 服务调用失败和重试
    service_logger->error(
        business_fields::event_type("service_call_failed"),
        FIELD("caller_service", "payment-service"),
        FIELD("target_service", "fraud-detection-service"),
        business_fields::http_method("POST"),
        FIELD("endpoint", "/api/v1/fraud/check"),
        business_fields::request_id("req_payment_999"),
        business_fields::error_code("SERVICE_TIMEOUT"),
        business_fields::error_message("Service did not respond within 5 seconds"),
        FIELD("timeout_ms", 5000),
        FIELD("retry_count", 2),
        FIELD("circuit_breaker_opened", true),
        FIELD("fallback_used", true)
    );
}

/**
 * @brief 主函数
 */
int main() {
    try {
        std::cout << "=== Zeus结构化日志业务场景示例 ===" << std::endl;
        
        // 创建日志目录
        std::filesystem::create_directories("logs");
        
        // 初始化结构化日志系统
        if (!InitializeStructuredLogging("", OutputFormat::JSON)) {
            std::cerr << "Failed to initialize structured logging" << std::endl;
            return 1;
        }
        
        // 打印框架信息
        std::cout << "使用Zeus结构化日志框架版本: " << GetVersion() << std::endl;
        
        // 运行各种业务场景模拟
        simulate_user_authentication();
        simulate_web_api_requests();
        simulate_payment_processing();
        simulate_database_operations();
        simulate_system_monitoring();
        simulate_microservice_communication();
        
        std::cout << "\n=== 业务场景示例完成 ===" << std::endl;
        std::cout << "所有业务场景的日志已记录到 logs/ 目录中。" << std::endl;
        std::cout << "这些示例展示了如何在实际项目中使用结构化日志来:" << std::endl;
        std::cout << "- 跟踪用户行为和认证事件" << std::endl;
        std::cout << "- 监控API性能和错误" << std::endl;
        std::cout << "- 记录支付和交易流程" << std::endl;
        std::cout << "- 诊断数据库性能问题" << std::endl;
        std::cout << "- 监控系统资源使用情况" << std::endl;
        std::cout << "- 跟踪微服务间的调用链" << std::endl;
        
        // 关闭日志系统
        ShutdownStructuredLogging();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "业务场景示例错误: " << e.what() << std::endl;
        return 1;
    }
}