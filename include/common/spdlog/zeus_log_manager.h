#pragma once

#include "zeus_log_common.h"
#include "zeus_log_config.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/hourly_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace common {
namespace spdlog {

class ZeusLogManager {
public:
    static ZeusLogManager& Instance();
    
    bool Initialize(const std::string& config_file = "");
    bool InitializeFromString(const std::string& json_config);
    
    std::shared_ptr<::spdlog::logger> GetLogger(const std::string& name);
    
    void SetGlobalLogLevel(LogLevel level);
    void Shutdown();

private:
    ZeusLogManager() = default;
    ~ZeusLogManager() = default;
    ZeusLogManager(const ZeusLogManager&) = delete;
    ZeusLogManager& operator=(const ZeusLogManager&) = delete;
    
    bool CreateLogger(const LoggerConfig& config);
    bool EnsureDirectoryExists(const std::string& path);
    std::string BuildLogFilePath(const LoggerConfig& config);
    
    std::unordered_map<std::string, std::shared_ptr<::spdlog::logger>> loggers_;
    std::mutex mutex_;
    bool initialized_{false};
};

// 便捷宏定义
#define ZEUS_LOG_MANAGER() common::spdlog::ZeusLogManager::Instance()
#define ZEUS_GET_LOGGER(name) ZEUS_LOG_MANAGER().GetLogger(name)

// 便捷日志宏
#define ZEUS_LOG_TRACE(logger_name, ...) if(auto logger = ZEUS_GET_LOGGER(logger_name)) logger->trace(__VA_ARGS__)
#define ZEUS_LOG_DEBUG(logger_name, ...) if(auto logger = ZEUS_GET_LOGGER(logger_name)) logger->debug(__VA_ARGS__)
#define ZEUS_LOG_INFO(logger_name, ...) if(auto logger = ZEUS_GET_LOGGER(logger_name)) logger->info(__VA_ARGS__)
#define ZEUS_LOG_WARN(logger_name, ...) if(auto logger = ZEUS_GET_LOGGER(logger_name)) logger->warn(__VA_ARGS__)
#define ZEUS_LOG_ERROR(logger_name, ...) if(auto logger = ZEUS_GET_LOGGER(logger_name)) logger->error(__VA_ARGS__)
#define ZEUS_LOG_CRITICAL(logger_name, ...) if(auto logger = ZEUS_GET_LOGGER(logger_name)) logger->critical(__VA_ARGS__)

} // namespace spdlog
} // namespace common