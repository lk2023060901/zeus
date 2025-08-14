#pragma once

/**
 * @file zeus_application.h
 * @brief Zeus Application Framework - Main include header
 * 
 * This header provides easy access to the complete Zeus Application Framework including:
 * - Application lifecycle management
 * - Configuration system with JSON support
 * - Dependency injection container
 * - Service factory and registry
 * - Network service integration (TCP/KCP/HTTP/HTTPS)
 * - Hook system for extensibility
 * - PostgreSQL and Redis configuration providers
 * 
 * Usage:
 * ```cpp
 * #include "core/zeus_application.h"
 * using namespace core::app;
 * 
 * int main() {
 *     auto& app = ZEUS_APP();
 *     
 *     // Register hooks
 *     app.RegisterStartupHook([](Application& app) {
 *         std::cout << "Application started!" << std::endl;
 *     });
 *     
 *     // Initialize and run
 *     if (app.Initialize("config.json")) {
 *         app.Run();
 *     }
 *     
 *     return 0;
 * }
 * ```
 */

// Core Application Framework
#include "app/application_types.h"
#include "app/application.h"
#include "app/app_config.h"
#include "app/dependency_injector.h"
#include "app/service_factory.h"
#include "app/service_registry.h"

// Configuration Providers
#include "app/config_providers/postgresql_config_provider.h"
#include "app/config_providers/redis_config_provider.h"

// Zeus Network Integration
#include "common/network/zeus_network.h"

namespace core {
namespace app {

/**
 * @brief Zeus Application Framework Version Information
 */
struct ZeusApplicationVersion {
    static constexpr int MAJOR = 1;
    static constexpr int MINOR = 0; 
    static constexpr int PATCH = 0;
    static constexpr const char* VERSION_STRING = "1.0.0";
    static constexpr const char* BUILD_TYPE = 
#ifdef ZEUS_CORE_APP_DEBUG
        "Debug"
#else
        "Release"
#endif
    ;
};

/**
 * @brief Application framework utilities and helper functions
 */
namespace ApplicationUtils {
    
    /**
     * @brief Create a default configuration file
     * @param filename Configuration file path
     * @param app_name Application name
     * @return true if created successfully
     */
    bool CreateDefaultConfig(const std::string& filename, const std::string& app_name = "zeus_app");
    
    /**
     * @brief Validate a configuration file
     * @param filename Configuration file path
     * @return true if valid
     */
    bool ValidateConfig(const std::string& filename);
    
    /**
     * @brief Get application framework version string
     */
    inline const char* GetFrameworkVersion() { return ZeusApplicationVersion::VERSION_STRING; }
    
    /**
     * @brief Get application framework build type
     */
    inline const char* GetBuildType() { return ZeusApplicationVersion::BUILD_TYPE; }
    
    /**
     * @brief Print framework information to console
     */
    void PrintFrameworkInfo();
    
    /**
     * @brief Create a simple HTTP echo server
     * @param port Port to listen on
     * @param bind_address Address to bind to
     * @return Listener configuration
     */
    ListenerConfig CreateHttpEchoServer(uint16_t port = 8080, const std::string& bind_address = "0.0.0.0");
    
    /**
     * @brief Create a simple TCP echo server
     * @param port Port to listen on
     * @param bind_address Address to bind to
     * @return Listener configuration
     */
    ListenerConfig CreateTcpEchoServer(uint16_t port = 9090, const std::string& bind_address = "0.0.0.0");
}

/**
 * @brief Predefined application hooks for common scenarios
 */
namespace CommonHooks {
    
    /**
     * @brief Hook that logs application startup information
     */
    void LogApplicationInfo(Application& app);
    
    /**
     * @brief Hook that prints service status on startup
     */
    void PrintServiceStatus(Application& app);
    
    /**
     * @brief Hook that sets up graceful shutdown
     */
    bool SetupGracefulShutdown(Application& app);
    
    /**
     * @brief Hook that initializes Lua scripting support
     */
    bool InitializeLuaSupport(Application& app);
    
    /**
     * @brief Hook that validates required services are running
     */
    void ValidateRequiredServices(Application& app);
    
    /**
     * @brief Hook that creates default HTTP routes
     */
    void SetupDefaultHttpRoutes(Application& app);
}

/**
 * @brief Configuration templates for common application types
 */
namespace ConfigTemplates {
    
    /**
     * @brief Create configuration for a simple web server
     */
    nlohmann::json WebServer(uint16_t http_port = 8080, bool enable_https = false, uint16_t https_port = 8443);
    
    /**
     * @brief Create configuration for a game server
     */
    nlohmann::json GameServer(uint16_t tcp_port = 9090, uint16_t kcp_port = 9091, uint16_t http_port = 8080);
    
    /**
     * @brief Create configuration for a microservice
     */
    nlohmann::json Microservice(const std::string& service_name, uint16_t http_port = 8080, bool enable_database = false);
    
    /**
     * @brief Create configuration for an API gateway
     */
    nlohmann::json ApiGateway(uint16_t http_port = 8080, uint16_t https_port = 8443);
    
    /**
     * @brief Create configuration for a chat server
     */
    nlohmann::json ChatServer(uint16_t tcp_port = 9090, uint16_t http_port = 8080, uint16_t websocket_port = 8081);
}

} // namespace app
} // namespace core

// Convenience macros for application development
#define ZEUS_FRAMEWORK_VERSION() core::app::ApplicationUtils::GetFrameworkVersion()
#define ZEUS_FRAMEWORK_BUILD_TYPE() core::app::ApplicationUtils::GetBuildType()
#define ZEUS_PRINT_INFO() core::app::ApplicationUtils::PrintFrameworkInfo()

// Hook registration macros
#define ZEUS_REGISTER_INIT_HOOK(hook) ZEUS_APP().RegisterInitHook(hook)
#define ZEUS_REGISTER_STARTUP_HOOK(hook) ZEUS_APP().RegisterStartupHook(hook)
#define ZEUS_REGISTER_SHUTDOWN_HOOK(hook) ZEUS_APP().RegisterShutdownHook(hook)

// Common setup macros
#define ZEUS_SETUP_LOGGING_HOOK() ZEUS_REGISTER_STARTUP_HOOK(core::app::CommonHooks::LogApplicationInfo)
#define ZEUS_SETUP_STATUS_HOOK() ZEUS_REGISTER_STARTUP_HOOK(core::app::CommonHooks::PrintServiceStatus)
#define ZEUS_SETUP_GRACEFUL_SHUTDOWN() ZEUS_REGISTER_INIT_HOOK(core::app::CommonHooks::SetupGracefulShutdown)
#define ZEUS_SETUP_DEFAULT_HTTP() ZEUS_REGISTER_STARTUP_HOOK(core::app::CommonHooks::SetupDefaultHttpRoutes)

// Quick application setup macro
#define ZEUS_QUICK_SETUP() do { \
    ZEUS_SETUP_GRACEFUL_SHUTDOWN(); \
    ZEUS_SETUP_LOGGING_HOOK(); \
    ZEUS_SETUP_STATUS_HOOK(); \
} while(0)