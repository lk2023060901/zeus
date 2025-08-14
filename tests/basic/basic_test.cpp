#include <iostream>
#include "common/spdlog/zeus_log_manager.h"

int main() {
    using namespace common::spdlog;
    
    // 从配置文件初始化
    if (!ZeusLogManager::Instance().Initialize("log_config.json")) {
        std::cerr << "Failed to initialize logging from config" << std::endl;
        return 1;
    }
    
    // 获取并测试game日志器
    auto game_logger = ZeusLogManager::Instance().GetLogger("game");
    if (game_logger) {
        game_logger->info("Zeus项目基础测试 - 日志系统正常工作");
        game_logger->warn("这是一个警告消息");
        game_logger->error("这是一个错误消息");
        
        std::cout << "Zeus基础测试完成 - 检查logs目录下的日志文件" << std::endl;
        return 0;
    } else {
        std::cerr << "Failed to get game logger" << std::endl;
        return 1;
    }
}