#include "common/network/network_events.h"
#include "common/network/network_logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace common {
namespace network {

NetworkEventManager& NetworkEventManager::Instance() {
    static NetworkEventManager instance;
    return instance;
}

std::string NetworkEventManager::RegisterHook(const std::vector<NetworkEventType>& event_types, const HookInfo& hook_info) {
    if (event_types.empty()) {
        NETWORK_LOG_WARN("Attempting to register hook '{}' with empty event types list", hook_info.name);
        return "";
    }

    std::lock_guard<std::mutex> lock(hooks_mutex_);
    
    // Check hook limits
    for (auto event_type : event_types) {
        if (max_hooks_per_type_ > 0 && event_hooks_[event_type].size() >= max_hooks_per_type_) {
            NETWORK_LOG_ERROR("Maximum number of hooks reached for event type {}, rejecting hook '{}'", 
                            static_cast<int>(event_type), hook_info.name);
            return "";
        }
    }

    std::string hook_id = GenerateHookId();
    auto registered_hook = std::make_shared<RegisteredHook>(hook_id, hook_info, event_types);
    
    // Add to event-specific hooks
    for (auto event_type : event_types) {
        event_hooks_[event_type].push_back(registered_hook);
        
        // Sort by priority (higher priority first)
        std::sort(event_hooks_[event_type].begin(), event_hooks_[event_type].end(),
                 [](const std::shared_ptr<RegisteredHook>& a, const std::shared_ptr<RegisteredHook>& b) {
                     return a->info.priority > b->info.priority;
                 });
    }
    
    // Add to registry
    hook_registry_[hook_id] = registered_hook;
    
    NETWORK_LOG_DEBUG("Registered hook '{}' with ID {} for {} event types", 
                     hook_info.name, hook_id, event_types.size());
    
    return hook_id;
}

std::string NetworkEventManager::RegisterGlobalHook(const HookInfo& hook_info) {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    
    std::string hook_id = GenerateHookId();
    auto registered_hook = std::make_shared<RegisteredHook>(hook_id, hook_info, std::vector<NetworkEventType>{});
    
    global_hooks_.push_back(registered_hook);
    
    // Sort global hooks by priority
    std::sort(global_hooks_.begin(), global_hooks_.end(),
             [](const std::shared_ptr<RegisteredHook>& a, const std::shared_ptr<RegisteredHook>& b) {
                 return a->info.priority > b->info.priority;
             });
    
    hook_registry_[hook_id] = registered_hook;
    
    NETWORK_LOG_DEBUG("Registered global hook '{}' with ID {}", hook_info.name, hook_id);
    
    return hook_id;
}

bool NetworkEventManager::UnregisterHook(const std::string& hook_id) {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    
    auto it = hook_registry_.find(hook_id);
    if (it == hook_registry_.end()) {
        NETWORK_LOG_WARN("Attempting to unregister non-existent hook ID: {}", hook_id);
        return false;
    }
    
    auto hook = it->second;
    
    // Remove from event-specific hooks
    for (auto event_type : hook->event_types) {
        auto& hooks = event_hooks_[event_type];
        hooks.erase(std::remove_if(hooks.begin(), hooks.end(),
                                  [hook_id](const std::shared_ptr<RegisteredHook>& h) {
                                      return h->id == hook_id;
                                  }), hooks.end());
    }
    
    // Remove from global hooks
    global_hooks_.erase(std::remove_if(global_hooks_.begin(), global_hooks_.end(),
                                      [hook_id](const std::shared_ptr<RegisteredHook>& h) {
                                          return h->id == hook_id;
                                      }), global_hooks_.end());
    
    // Remove from registry
    hook_registry_.erase(it);
    
    NETWORK_LOG_DEBUG("Unregistered hook '{}' with ID {}", hook->info.name, hook_id);
    
    return true;
}

void NetworkEventManager::UnregisterHooks(const std::vector<NetworkEventType>& event_types) {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    
    size_t removed_count = 0;
    for (auto event_type : event_types) {
        auto& hooks = event_hooks_[event_type];
        
        // Remove hooks from registry
        for (const auto& hook : hooks) {
            hook_registry_.erase(hook->id);
            removed_count++;
        }
        
        hooks.clear();
    }
    
    NETWORK_LOG_DEBUG("Unregistered {} hooks for {} event types", removed_count, event_types.size());
}

void NetworkEventManager::ClearAllHooks() {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    
    size_t total_hooks = hook_registry_.size();
    
    event_hooks_.clear();
    global_hooks_.clear();
    hook_registry_.clear();
    
    NETWORK_LOG_DEBUG("Cleared all {} registered hooks", total_hooks);
}

void NetworkEventManager::FireEvent(const NetworkEvent& event) {
    if (!event_processing_enabled_) {
        return;
    }
    
    ProcessEvent(event);
}

void NetworkEventManager::FireEventAsync(const NetworkEvent& event, boost::asio::any_io_executor executor) {
    if (!event_processing_enabled_) {
        return;
    }
    
    boost::asio::post(executor, [this, event]() {
        ProcessEvent(event);
    });
}

void NetworkEventManager::ProcessEvent(const NetworkEvent& event) {
    std::vector<std::shared_ptr<RegisteredHook>> hooks_to_call;
    std::vector<std::shared_ptr<RegisteredHook>> hooks_to_remove;
    
    {
        std::lock_guard<std::mutex> lock(hooks_mutex_);
        
        // Collect global hooks
        for (auto& hook : global_hooks_) {
            if (!hook->info.filter || hook->info.filter(event)) {
                hooks_to_call.push_back(hook);
                if (hook->info.once) {
                    hooks_to_remove.push_back(hook);
                }
            }
        }
        
        // Collect event-specific hooks
        auto it = event_hooks_.find(event.type);
        if (it != event_hooks_.end()) {
            for (auto& hook : it->second) {
                if (!hook->info.filter || hook->info.filter(event)) {
                    hooks_to_call.push_back(hook);
                    if (hook->info.once) {
                        hooks_to_remove.push_back(hook);
                    }
                }
            }
        }
    }
    
    // Execute hooks (outside of lock to avoid deadlock)
    for (auto& hook : hooks_to_call) {
        try {
            hook->info.hook(event);
            hook->call_count++;
            
            NETWORK_LOG_TRACE("Executed hook '{}' for event type {}", 
                            hook->info.name, static_cast<int>(event.type));
        } catch (const std::exception& e) {
            NETWORK_LOG_ERROR("Exception in hook '{}': {}", hook->info.name, e.what());
        } catch (...) {
            NETWORK_LOG_ERROR("Unknown exception in hook '{}'", hook->info.name);
        }
    }
    
    // Remove one-time hooks
    for (auto& hook : hooks_to_remove) {
        UnregisterHook(hook->id);
    }
}

void NetworkEventManager::SetEventProcessingEnabled(bool enable) {
    event_processing_enabled_ = enable;
    NETWORK_LOG_INFO("Network event processing {}", enable ? "enabled" : "disabled");
}

bool NetworkEventManager::IsEventProcessingEnabled() const {
    return event_processing_enabled_;
}

std::unordered_map<NetworkEventType, size_t> NetworkEventManager::GetHookStatistics() const {
    std::lock_guard<std::mutex> lock(hooks_mutex_);
    
    std::unordered_map<NetworkEventType, size_t> stats;
    
    for (const auto& [event_type, hooks] : event_hooks_) {
        stats[event_type] = hooks.size();
    }
    
    return stats;
}

void NetworkEventManager::SetMaxHooksPerType(size_t max_hooks) {
    max_hooks_per_type_ = max_hooks;
    NETWORK_LOG_INFO("Set maximum hooks per event type to {}", max_hooks);
}

std::string NetworkEventManager::GenerateHookId() {
    std::ostringstream oss;
    oss << "hook_" << std::setfill('0') << std::setw(8) << next_hook_id_.fetch_add(1);
    return oss.str();
}

} // namespace network
} // namespace common