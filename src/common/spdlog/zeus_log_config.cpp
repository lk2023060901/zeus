#include "common/spdlog/zeus_log_config.h"
#include "common/spdlog/zeus_log_common.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace common {
namespace spdlog {

ZeusLogConfig& ZeusLogConfig::Instance() {
    static ZeusLogConfig instance;
    return instance;
}

bool ZeusLogConfig::LoadFromFile(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        std::cerr << "Failed to open log config file: " << config_file << std::endl;
        return false;
    }
    
    std::string json_content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();
    
    return LoadFromString(json_content);
}

bool ZeusLogConfig::LoadFromString(const std::string& json_content) {
    try {
        return ParseJsonConfig(json_content);
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse log config: " << e.what() << std::endl;
        return false;
    }
}

const std::vector<LoggerConfig>& ZeusLogConfig::GetLoggerConfigs() const {
    return logger_configs_;
}

const LoggerConfig* ZeusLogConfig::GetLoggerConfig(const std::string& name) const {
    for (const auto& config : logger_configs_) {
        if (config.name == name) {
            return &config;
        }
    }
    return nullptr;
}

void ZeusLogConfig::SetGlobalLogLevel(LogLevel level) {
    global_log_level_ = level;
}

LogLevel ZeusLogConfig::GetGlobalLogLevel() const {
    return global_log_level_;
}

void ZeusLogConfig::SetGlobalLogDir(const std::string& log_dir) {
    global_log_dir_ = log_dir;
}

const std::string& ZeusLogConfig::GetGlobalLogDir() const {
    return global_log_dir_;
}

bool ZeusLogConfig::ParseJsonConfig(const std::string& json_content) {
    nlohmann::json config = nlohmann::json::parse(json_content);
    
    // 解析全局配置
    if (config.contains("global")) {
        const auto& global = config["global"];
        if (global.contains("log_level")) {
            global_log_level_ = ParseLogLevel(global["log_level"]);
        }
        if (global.contains("log_dir")) {
            global_log_dir_ = global["log_dir"];
        }
    }
    
    // 解析日志器配置
    if (config.contains("loggers") && config["loggers"].is_array()) {
        logger_configs_.clear();
        for (const auto& logger_json : config["loggers"]) {
            LoggerConfig logger_config;
            
            if (logger_json.contains("name")) {
                logger_config.name = logger_json["name"];
            }
            
            if (logger_json.contains("log_dir")) {
                logger_config.log_dir = logger_json["log_dir"];
            } else {
                logger_config.log_dir = global_log_dir_;
            }
            
            if (logger_json.contains("filename_pattern")) {
                logger_config.filename_pattern = logger_json["filename_pattern"];
            } else {
                logger_config.filename_pattern = logger_config.name + ".log";
            }
            
            if (logger_json.contains("level")) {
                logger_config.level = ParseLogLevel(logger_json["level"]);
            } else {
                logger_config.level = global_log_level_;
            }
            
            if (logger_json.contains("rotation_type")) {
                logger_config.rotation_type = ParseRotationType(logger_json["rotation_type"]);
            }
            
            if (logger_json.contains("console_output")) {
                logger_config.console_output = logger_json["console_output"];
            }
            
            logger_configs_.push_back(logger_config);
        }
    }
    
    return true;
}

LogLevel ZeusLogConfig::ParseLogLevel(const std::string& level_str) const {
    if (level_str == "trace") return LogLevel::TRACE;
    if (level_str == "debug") return LogLevel::DEBUG;
    if (level_str == "info") return LogLevel::INFO;
    if (level_str == "warn") return LogLevel::WARN;
    if (level_str == "error") return LogLevel::ERROR;
    if (level_str == "critical") return LogLevel::CRITICAL;
    if (level_str == "off") return LogLevel::OFF;
    return LogLevel::INFO;
}

RotationType ZeusLogConfig::ParseRotationType(const std::string& rotation_str) const {
    if (rotation_str == "daily") return RotationType::DAILY;
    if (rotation_str == "hourly") return RotationType::HOURLY;
    return RotationType::DAILY;
}

::spdlog::level::level_enum ToSpdlogLevel(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return ::spdlog::level::trace;
        case LogLevel::DEBUG: return ::spdlog::level::debug;
        case LogLevel::INFO: return ::spdlog::level::info;
        case LogLevel::WARN: return ::spdlog::level::warn;
        case LogLevel::ERROR: return ::spdlog::level::err;
        case LogLevel::CRITICAL: return ::spdlog::level::critical;
        case LogLevel::OFF: return ::spdlog::level::off;
        default: return ::spdlog::level::info;
    }
}

LogLevel FromSpdlogLevel(::spdlog::level::level_enum level) {
    switch (level) {
        case ::spdlog::level::trace: return LogLevel::TRACE;
        case ::spdlog::level::debug: return LogLevel::DEBUG;
        case ::spdlog::level::info: return LogLevel::INFO;
        case ::spdlog::level::warn: return LogLevel::WARN;
        case ::spdlog::level::err: return LogLevel::ERROR;
        case ::spdlog::level::critical: return LogLevel::CRITICAL;
        case ::spdlog::level::off: return LogLevel::OFF;
        default: return LogLevel::INFO;
    }
}

} // namespace spdlog
} // namespace common