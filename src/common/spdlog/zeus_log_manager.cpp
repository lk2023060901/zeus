#include "common/spdlog/zeus_log_manager.h"
#include <filesystem>
#include <iostream>

namespace common {
namespace spdlog {

ZeusLogManager& ZeusLogManager::Instance() {
    static ZeusLogManager instance;
    return instance;
}

bool ZeusLogManager::Initialize(const std::string& config_file) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return true;
    }
    
    auto& config = ZeusLogConfig::Instance();
    
    bool load_success = false;
    if (!config_file.empty()) {
        load_success = config.LoadFromFile(config_file);
    } else {
        // 使用默认配置
        std::string default_config = R"({
            "global": {
                "log_level": "info",
                "log_dir": "logs"
            },
            "loggers": [
                {
                    "name": "default",
                    "rotation_type": "daily",
                    "console_output": true
                }
            ]
        })";
        load_success = config.LoadFromString(default_config);
    }
    
    if (!load_success) {
        std::cerr << "Failed to load log configuration" << std::endl;
        return false;
    }
    
    // 创建配置中定义的所有日志器
    for (const auto& logger_config : config.GetLoggerConfigs()) {
        if (!CreateLogger(logger_config)) {
            std::cerr << "Failed to create logger: " << logger_config.name << std::endl;
            return false;
        }
    }
    
    initialized_ = true;
    return true;
}

bool ZeusLogManager::InitializeFromString(const std::string& json_config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return true;
    }
    
    auto& config = ZeusLogConfig::Instance();
    if (!config.LoadFromString(json_config)) {
        return false;
    }
    
    // 创建配置中定义的所有日志器
    for (const auto& logger_config : config.GetLoggerConfigs()) {
        if (!CreateLogger(logger_config)) {
            std::cerr << "Failed to create logger: " << logger_config.name << std::endl;
            return false;
        }
    }
    
    initialized_ = true;
    return true;
}

std::shared_ptr<::spdlog::logger> ZeusLogManager::GetLogger(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loggers_.find(name);
    if (it != loggers_.end()) {
        return it->second;
    }
    
    // 如果没找到，尝试创建默认配置的日志器
    LoggerConfig default_config;
    default_config.name = name;
    default_config.log_dir = ZeusLogConfig::Instance().GetGlobalLogDir();
    default_config.filename_pattern = name + ".log";
    default_config.level = ZeusLogConfig::Instance().GetGlobalLogLevel();
    default_config.rotation_type = RotationType::DAILY;
    default_config.console_output = false;
    
    if (CreateLogger(default_config)) {
        return loggers_[name];
    }
    
    return nullptr;
}

void ZeusLogManager::SetGlobalLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    ZeusLogConfig::Instance().SetGlobalLogLevel(level);
    
    // 更新所有现有日志器的级别
    for (auto& pair : loggers_) {
        pair.second->set_level(ToSpdlogLevel(level));
    }
}

void ZeusLogManager::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : loggers_) {
        pair.second->flush();
    }
    
    loggers_.clear();
    ::spdlog::shutdown();
    initialized_ = false;
}

bool ZeusLogManager::CreateLogger(const LoggerConfig& config) {
    try {
        // 确保日志目录存在
        if (!EnsureDirectoryExists(config.log_dir)) {
            std::cerr << "Failed to create log directory: " << config.log_dir << std::endl;
            return false;
        }
        
        std::string log_file_path = BuildLogFilePath(config);
        
        std::vector<::spdlog::sink_ptr> sinks;
        
        // 添加文件输出sink
        if (config.rotation_type == RotationType::DAILY) {
            auto file_sink = std::make_shared<::spdlog::sinks::daily_file_sink_mt>(
                log_file_path, 0, 0  // 每天0点0分创建新文件
            );
            sinks.push_back(file_sink);
        } else if (config.rotation_type == RotationType::HOURLY) {
            auto file_sink = std::make_shared<::spdlog::sinks::hourly_file_sink_mt>(
                log_file_path, false  // 不截断现有文件
            );
            sinks.push_back(file_sink);
        }
        
        // 添加控制台输出sink（如果配置了）
        if (config.console_output) {
            auto console_sink = std::make_shared<::spdlog::sinks::stdout_color_sink_mt>();
            sinks.push_back(console_sink);
        }
        
        // 创建日志器
        auto logger = std::make_shared<::spdlog::logger>(config.name, sinks.begin(), sinks.end());
        logger->set_level(ToSpdlogLevel(config.level));
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
        
        // 注册日志器
        ::spdlog::register_logger(logger);
        loggers_[config.name] = logger;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception creating logger " << config.name << ": " << e.what() << std::endl;
        return false;
    }
}

bool ZeusLogManager::EnsureDirectoryExists(const std::string& path) {
    try {
        std::filesystem::path dir_path(path);
        if (!std::filesystem::exists(dir_path)) {
            return std::filesystem::create_directories(dir_path);
        }
        return std::filesystem::is_directory(dir_path);
    } catch (const std::exception& e) {
        std::cerr << "Failed to create directory " << path << ": " << e.what() << std::endl;
        return false;
    }
}

std::string ZeusLogManager::BuildLogFilePath(const LoggerConfig& config) {
    std::filesystem::path log_path(config.log_dir);
    log_path /= config.filename_pattern;
    return log_path.string();
}

} // namespace spdlog
} // namespace common