#pragma once

#include "zeus_log_common.h"
#include <vector>
#include <string>

namespace common {
namespace spdlog {

class ZeusLogConfig {
public:
    static ZeusLogConfig& Instance();
    
    bool LoadFromFile(const std::string& config_file);
    bool LoadFromString(const std::string& json_content);
    
    const std::vector<LoggerConfig>& GetLoggerConfigs() const;
    const LoggerConfig* GetLoggerConfig(const std::string& name) const;
    
    void SetGlobalLogLevel(LogLevel level);
    LogLevel GetGlobalLogLevel() const;
    
    void SetGlobalLogDir(const std::string& log_dir);
    const std::string& GetGlobalLogDir() const;

private:
    ZeusLogConfig() = default;
    ~ZeusLogConfig() = default;
    ZeusLogConfig(const ZeusLogConfig&) = delete;
    ZeusLogConfig& operator=(const ZeusLogConfig&) = delete;
    
    bool ParseJsonConfig(const std::string& json_content);
    LogLevel ParseLogLevel(const std::string& level_str) const;
    RotationType ParseRotationType(const std::string& rotation_str) const;
    
    std::vector<LoggerConfig> logger_configs_;
    LogLevel global_log_level_{LogLevel::INFO};
    std::string global_log_dir_{"logs"};
};

} // namespace spdlog
} // namespace common