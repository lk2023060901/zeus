#include "common/network/network_logger.h"
#include <sstream>
#include <iomanip>

namespace common {
namespace network {

NetworkLogger& NetworkLogger::Instance() {
    static NetworkLogger instance;
    return instance;
}

bool NetworkLogger::Initialize(const std::string& logger_name, bool enable_logging) {
    if (initialized_) {
        return true;
    }

    logger_name_ = logger_name;
    logging_enabled_ = enable_logging;

    if (!logging_enabled_) {
        initialized_ = true;
        return true;
    }

    // Get logger from Zeus log manager
    logger_ = common::spdlog::ZeusLogManager::Instance().GetLogger(logger_name_);
    if (!logger_) {
        // Fallback: try to get a default logger or create one
        logger_ = ::spdlog::get(logger_name_);
        if (!logger_) {
            // Create a basic console logger as fallback
            logger_ = ::spdlog::stdout_color_mt(logger_name_);
            if (logger_) {
                logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
                logger_->set_level(::spdlog::level::info);
            }
        }
    }

    initialized_ = (logger_ != nullptr);
    return initialized_;
}

std::shared_ptr<::spdlog::logger> NetworkLogger::GetLogger() const {
    if (!logging_enabled_ || !initialized_) {
        return nullptr;
    }
    return logger_;
}

void NetworkLogger::SetLoggingEnabled(bool enable) {
    logging_enabled_ = enable;
}

bool NetworkLogger::IsLoggingEnabled() const {
    return logging_enabled_ && initialized_ && (logger_ != nullptr);
}

void NetworkLogger::LogConnection(const std::string& connection_id, const std::string& endpoint, const std::string& protocol) {
    if (!IsLoggingEnabled()) return;
    
    logger_->info("CONNECTION_ESTABLISHED - ID: {}, Endpoint: {}, Protocol: {}", 
                  connection_id, endpoint, protocol);
}

void NetworkLogger::LogDisconnection(const std::string& connection_id, const std::string& reason) {
    if (!IsLoggingEnabled()) return;
    
    logger_->info("CONNECTION_TERMINATED - ID: {}, Reason: {}", 
                  connection_id, reason);
}

void NetworkLogger::LogDataTransfer(const std::string& connection_id, const std::string& direction, 
                                   size_t bytes_count, const std::string& data_type) {
    if (!IsLoggingEnabled()) return;
    
    if (data_type.empty()) {
        logger_->debug("DATA_TRANSFER - ID: {}, Direction: {}, Bytes: {}", 
                      connection_id, direction, bytes_count);
    } else {
        logger_->debug("DATA_TRANSFER - ID: {}, Direction: {}, Bytes: {}, Type: {}", 
                      connection_id, direction, bytes_count, data_type);
    }
}

void NetworkLogger::LogError(const std::string& connection_id, int error_code, const std::string& error_message) {
    if (!IsLoggingEnabled()) return;
    
    if (connection_id.empty()) {
        logger_->error("NETWORK_ERROR - Code: {}, Message: {}", error_code, error_message);
    } else {
        logger_->error("NETWORK_ERROR - ID: {}, Code: {}, Message: {}", 
                      connection_id, error_code, error_message);
    }
}

void NetworkLogger::LogPerformance(const std::string& connection_id, const std::string& metric_name, 
                                  double value, const std::string& unit) {
    if (!IsLoggingEnabled()) return;
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << value;
    
    logger_->debug("PERFORMANCE_METRIC - ID: {}, Metric: {}, Value: {}{}", 
                  connection_id, metric_name, oss.str(), unit);
}

void NetworkLogger::Shutdown() {
    if (logger_) {
        logger_->flush();
        logger_.reset();
    }
    initialized_ = false;
}

} // namespace network
} // namespace common