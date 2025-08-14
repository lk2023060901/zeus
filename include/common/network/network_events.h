#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <boost/asio.hpp>

namespace common {
namespace network {

// Forward declarations
class Connection;

/**
 * @brief Network event types
 */
enum class NetworkEventType {
    CONNECTION_ESTABLISHED,     // New connection established
    CONNECTION_FAILED,         // Connection attempt failed
    CONNECTION_CLOSED,         // Connection closed gracefully
    CONNECTION_ERROR,          // Connection error occurred
    DATA_RECEIVED,             // Data received
    DATA_SENT,                 // Data sent successfully
    DATA_SEND_ERROR,           // Data send failed
    PROTOCOL_ERROR,            // Protocol-level error
    HEARTBEAT_TIMEOUT,         // Heartbeat timeout
    CUSTOM                     // Custom user-defined event
};

/**
 * @brief Network event data structure
 */
struct NetworkEvent {
    NetworkEventType type;
    std::string connection_id;
    std::string endpoint;
    std::string protocol;
    std::shared_ptr<Connection> connection;
    
    // Data-related fields
    std::vector<uint8_t> data;
    size_t bytes_transferred = 0;
    
    // Error-related fields
    boost::system::error_code error_code;
    std::string error_message;
    
    // Custom fields
    std::unordered_map<std::string, std::string> custom_data;
    
    // Timestamp
    std::chrono::steady_clock::time_point timestamp;
    
    NetworkEvent(NetworkEventType event_type = NetworkEventType::CUSTOM) 
        : type(event_type), timestamp(std::chrono::steady_clock::now()) {}
};

/**
 * @brief Hook function types
 */
using NetworkEventHook = std::function<void(const NetworkEvent&)>;
using NetworkEventFilter = std::function<bool(const NetworkEvent&)>;

/**
 * @brief Hook registration information
 */
struct HookInfo {
    std::string name;
    NetworkEventHook hook;
    NetworkEventFilter filter;
    int priority = 0; // Higher priority hooks are called first
    bool once = false; // If true, hook is removed after first call
    
    HookInfo(const std::string& hook_name, NetworkEventHook hook_func, 
             NetworkEventFilter filter_func = nullptr, int hook_priority = 0, bool call_once = false)
        : name(hook_name), hook(std::move(hook_func)), filter(std::move(filter_func)), 
          priority(hook_priority), once(call_once) {}
};

/**
 * @brief Network event manager for handling hooks and event dispatching
 */
class NetworkEventManager {
public:
    /**
     * @brief Get singleton instance
     */
    static NetworkEventManager& Instance();

    /**
     * @brief Register a hook for specific event types
     * @param event_types Set of event types to listen for
     * @param hook_info Hook information
     * @return Hook ID for later removal
     */
    std::string RegisterHook(const std::vector<NetworkEventType>& event_types, const HookInfo& hook_info);

    /**
     * @brief Register a hook for all event types
     * @param hook_info Hook information  
     * @return Hook ID for later removal
     */
    std::string RegisterGlobalHook(const HookInfo& hook_info);

    /**
     * @brief Unregister a hook by ID
     * @param hook_id Hook ID returned by RegisterHook
     * @return true if hook was found and removed
     */
    bool UnregisterHook(const std::string& hook_id);

    /**
     * @brief Unregister all hooks for specific event types
     * @param event_types Event types to clear hooks for
     */
    void UnregisterHooks(const std::vector<NetworkEventType>& event_types);

    /**
     * @brief Unregister all hooks
     */
    void ClearAllHooks();

    /**
     * @brief Fire an event to all registered hooks
     * @param event Event to fire
     */
    void FireEvent(const NetworkEvent& event);

    /**
     * @brief Fire an event asynchronously
     * @param event Event to fire
     * @param executor Asio executor for async execution
     */
    void FireEventAsync(const NetworkEvent& event, boost::asio::any_io_executor executor);

    /**
     * @brief Enable/disable event processing
     * @param enable Whether to enable event processing
     */
    void SetEventProcessingEnabled(bool enable);

    /**
     * @brief Check if event processing is enabled
     */
    bool IsEventProcessingEnabled() const;

    /**
     * @brief Get statistics about hook registrations
     */
    std::unordered_map<NetworkEventType, size_t> GetHookStatistics() const;

    /**
     * @brief Set maximum number of hooks per event type (for safety)
     * @param max_hooks Maximum number of hooks (0 = unlimited)
     */
    void SetMaxHooksPerType(size_t max_hooks);

private:
    NetworkEventManager() = default;
    ~NetworkEventManager() = default;
    NetworkEventManager(const NetworkEventManager&) = delete;
    NetworkEventManager& operator=(const NetworkEventManager&) = delete;

    struct RegisteredHook {
        std::string id;
        HookInfo info;
        std::vector<NetworkEventType> event_types;
        size_t call_count = 0;
        std::chrono::steady_clock::time_point registered_at;
        
        RegisteredHook(const std::string& hook_id, const HookInfo& hook_info, 
                      const std::vector<NetworkEventType>& types)
            : id(hook_id), info(hook_info), event_types(types), 
              registered_at(std::chrono::steady_clock::now()) {}
    };

    std::string GenerateHookId();
    void ProcessEvent(const NetworkEvent& event);
    
    mutable std::mutex hooks_mutex_;
    std::unordered_map<NetworkEventType, std::vector<std::shared_ptr<RegisteredHook>>> event_hooks_;
    std::vector<std::shared_ptr<RegisteredHook>> global_hooks_;
    std::unordered_map<std::string, std::shared_ptr<RegisteredHook>> hook_registry_;
    
    bool event_processing_enabled_ = true;
    size_t max_hooks_per_type_ = 100; // Safety limit
    std::atomic<size_t> next_hook_id_{1};
};

/**
 * @brief Utility functions for creating common event filters
 */
namespace EventFilters {
    /**
     * @brief Filter by connection ID
     */
    inline NetworkEventFilter ByConnectionId(const std::string& connection_id) {
        return [connection_id](const NetworkEvent& event) {
            return event.connection_id == connection_id;
        };
    }

    /**
     * @brief Filter by protocol type
     */
    inline NetworkEventFilter ByProtocol(const std::string& protocol) {
        return [protocol](const NetworkEvent& event) {
            return event.protocol == protocol;
        };
    }

    /**
     * @brief Filter by endpoint pattern
     */
    inline NetworkEventFilter ByEndpointPattern(const std::string& pattern) {
        return [pattern](const NetworkEvent& event) {
            return event.endpoint.find(pattern) != std::string::npos;
        };
    }

    /**
     * @brief Filter by minimum data size
     */
    inline NetworkEventFilter ByMinDataSize(size_t min_size) {
        return [min_size](const NetworkEvent& event) {
            return event.bytes_transferred >= min_size;
        };
    }

    /**
     * @brief Combine multiple filters with AND logic
     */
    template<typename... Filters>
    inline NetworkEventFilter And(Filters... filters) {
        return [filters...](const NetworkEvent& event) {
            return (filters(event) && ...);
        };
    }

    /**
     * @brief Combine multiple filters with OR logic
     */
    template<typename... Filters>
    inline NetworkEventFilter Or(Filters... filters) {
        return [filters...](const NetworkEvent& event) {
            return (filters(event) || ...);
        };
    }
}

/**
 * @brief Utility macros for easy hook registration
 */
#define REGISTER_NETWORK_HOOK(event_types, name, hook_func) \
    NetworkEventManager::Instance().RegisterHook(event_types, HookInfo(name, hook_func))

#define REGISTER_NETWORK_HOOK_WITH_FILTER(event_types, name, hook_func, filter_func) \
    NetworkEventManager::Instance().RegisterHook(event_types, HookInfo(name, hook_func, filter_func))

#define REGISTER_GLOBAL_NETWORK_HOOK(name, hook_func) \
    NetworkEventManager::Instance().RegisterGlobalHook(HookInfo(name, hook_func))

} // namespace network
} // namespace common