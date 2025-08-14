#pragma once

#include <memory>
#include <string>
#include "common/spdlog/zeus_log_manager.h"

// Compilation macros for network logging
#ifndef ZEUS_NETWORK_LOGGING_ENABLED
#define ZEUS_NETWORK_LOGGING_ENABLED 1
#endif

// Network logging macros
#if ZEUS_NETWORK_LOGGING_ENABLED
#define NETWORK_LOG_TRACE(msg, ...) \
    do { \
        if (auto logger = common::network::NetworkLogger::Instance().GetLogger()) { \
            logger->trace(msg, ##__VA_ARGS__); \
        } \
    } while(0)

#define NETWORK_LOG_DEBUG(msg, ...) \
    do { \
        if (auto logger = common::network::NetworkLogger::Instance().GetLogger()) { \
            logger->debug(msg, ##__VA_ARGS__); \
        } \
    } while(0)

#define NETWORK_LOG_INFO(msg, ...) \
    do { \
        if (auto logger = common::network::NetworkLogger::Instance().GetLogger()) { \
            logger->info(msg, ##__VA_ARGS__); \
        } \
    } while(0)

#define NETWORK_LOG_WARN(msg, ...) \
    do { \
        if (auto logger = common::network::NetworkLogger::Instance().GetLogger()) { \
            logger->warn(msg, ##__VA_ARGS__); \
        } \
    } while(0)

#define NETWORK_LOG_ERROR(msg, ...) \
    do { \
        if (auto logger = common::network::NetworkLogger::Instance().GetLogger()) { \
            logger->error(msg, ##__VA_ARGS__); \
        } \
    } while(0)

#define NETWORK_LOG_CRITICAL(msg, ...) \
    do { \
        if (auto logger = common::network::NetworkLogger::Instance().GetLogger()) { \
            logger->critical(msg, ##__VA_ARGS__); \
        } \
    } while(0)

#else
// When logging is disabled, all macros are no-ops
#define NETWORK_LOG_TRACE(msg, ...) ((void)0)
#define NETWORK_LOG_DEBUG(msg, ...) ((void)0)
#define NETWORK_LOG_INFO(msg, ...) ((void)0)
#define NETWORK_LOG_WARN(msg, ...) ((void)0)
#define NETWORK_LOG_ERROR(msg, ...) ((void)0)
#define NETWORK_LOG_CRITICAL(msg, ...) ((void)0)
#endif

namespace common {
namespace network {

/**
 * @brief Network module dedicated logger
 * 
 * Provides centralized logging for all network operations including:
 * - TCP connection lifecycle
 * - KCP protocol operations
 * - Data transmission/reception
 * - Error handling and diagnostics
 * - Performance metrics
 */
class NetworkLogger {
public:
    /**
     * @brief Get singleton instance
     * @return Reference to NetworkLogger instance
     */
    static NetworkLogger& Instance();

    /**
     * @brief Initialize network logging system
     * @param logger_name Name of the logger in configuration
     * @param enable_logging Whether to enable logging (can override compile-time macro)
     * @return true if initialization successful, false otherwise
     */
    bool Initialize(const std::string& logger_name = "network", bool enable_logging = true);

    /**
     * @brief Get the underlying logger instance
     * @return Shared pointer to spdlog logger, may be null if disabled
     */
    std::shared_ptr<::spdlog::logger> GetLogger() const;

    /**
     * @brief Enable/disable network logging at runtime
     * @param enable Whether to enable logging
     */
    void SetLoggingEnabled(bool enable);

    /**
     * @brief Check if logging is currently enabled
     * @return true if enabled, false otherwise
     */
    bool IsLoggingEnabled() const;

    /**
     * @brief Log connection establishment
     * @param connection_id Unique connection identifier
     * @param endpoint Remote endpoint information
     * @param protocol Protocol type (TCP/KCP)
     */
    void LogConnection(const std::string& connection_id, const std::string& endpoint, const std::string& protocol);

    /**
     * @brief Log connection termination
     * @param connection_id Unique connection identifier
     * @param reason Disconnection reason
     */
    void LogDisconnection(const std::string& connection_id, const std::string& reason);

    /**
     * @brief Log data transmission
     * @param connection_id Connection identifier
     * @param direction "send" or "receive"
     * @param bytes_count Number of bytes
     * @param data_type Optional data type description
     */
    void LogDataTransfer(const std::string& connection_id, const std::string& direction, 
                        size_t bytes_count, const std::string& data_type = "");

    /**
     * @brief Log network errors
     * @param connection_id Connection identifier (can be empty for general errors)
     * @param error_code Error code
     * @param error_message Error description
     */
    void LogError(const std::string& connection_id, int error_code, const std::string& error_message);

    /**
     * @brief Log performance metrics
     * @param connection_id Connection identifier
     * @param metric_name Metric name (latency, throughput, etc.)
     * @param value Metric value
     * @param unit Unit of measurement
     */
    void LogPerformance(const std::string& connection_id, const std::string& metric_name, 
                       double value, const std::string& unit);

    /**
     * @brief Shutdown logging system
     */
    void Shutdown();

private:
    NetworkLogger() = default;
    ~NetworkLogger() = default;
    NetworkLogger(const NetworkLogger&) = delete;
    NetworkLogger& operator=(const NetworkLogger&) = delete;

    std::shared_ptr<::spdlog::logger> logger_;
    std::string logger_name_;
    bool initialized_ = false;
    bool logging_enabled_ = true;
};

} // namespace network
} // namespace common