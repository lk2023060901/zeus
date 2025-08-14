#include "core/app/dependency_injector.h"
#include <mutex>

namespace core {
namespace app {

bool DependencyInjector::HasConfigProvider(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_providers_.find(name) != config_providers_.end();
}

std::vector<std::string> DependencyInjector::GetConfigProviderNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(config_providers_.size());
    
    for (const auto& pair : config_providers_) {
        names.push_back(pair.first);
    }
    
    return names;
}

std::vector<std::string> DependencyInjector::GetRegisteredServices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> services;
    services.reserve(services_.size());
    
    for (const auto& pair : services_) {
        std::string info = pair.first;
        auto type_it = service_type_names_.find(pair.first);
        if (type_it != service_type_names_.end()) {
            info += " (" + type_it->second + ")";
        }
        info += pair.second.is_singleton ? " [Singleton]" : " [Factory]";
        services.push_back(info);
    }
    
    return services;
}

void DependencyInjector::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    config_providers_.clear();
    services_.clear();
    service_type_names_.clear();
}

} // namespace app
} // namespace core