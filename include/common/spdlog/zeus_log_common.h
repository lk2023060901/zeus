#pragma once

#include <string>
#include <memory>
#include <spdlog/spdlog.h>

namespace common {
namespace spdlog {

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5,
    OFF = 6
};

enum class RotationType {
    DAILY,    // 按天分割
    HOURLY    // 按小时分割
};

struct LoggerConfig {
    std::string name;
    std::string log_dir;
    std::string filename_pattern;
    LogLevel level;
    RotationType rotation_type;
    bool console_output;
    
    LoggerConfig() 
        : level(LogLevel::INFO)
        , rotation_type(RotationType::DAILY)
        , console_output(false) {}
};

::spdlog::level::level_enum ToSpdlogLevel(LogLevel level);
LogLevel FromSpdlogLevel(::spdlog::level::level_enum level);

} // namespace spdlog
} // namespace common