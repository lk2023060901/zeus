/**
 * @file enhanced_signal_demo.cpp
 * @brief 演示增强信号处理机制的示例程序
 * 
 * 这个示例展示了如何使用Zeus Application框架的增强信号处理功能，
 * 包括Hook、Lambda和Handler参数的不同使用方式。
 */

#include <iostream>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <csignal>

// 模拟Zeus Application框架的信号处理类型定义
namespace enhanced_signal_demo {

// 前向声明
class MockApplication;

// 信号处理类型定义
using SignalHook = std::function<void(MockApplication&, int signal)>;
using SignalHandler = std::function<bool(MockApplication&, int signal)>; // 返回true表示继续默认处理

/**
 * @brief 信号处理策略枚举
 */
enum class SignalHandlerStrategy {
    DEFAULT_ONLY,      // 仅使用默认处理
    HOOK_FIRST,       // Hook优先，然后默认处理
    HOOK_ONLY,        // 仅使用Hook处理
    HOOK_OVERRIDE     // Hook决定是否继续默认处理
};

/**
 * @brief 信号处理配置结构
 */
struct SignalHandlerConfig {
    SignalHandlerStrategy strategy = SignalHandlerStrategy::DEFAULT_ONLY;
    std::vector<int> handled_signals = {SIGINT, SIGTERM}; // 默认处理的信号
    bool graceful_shutdown = true;     // 是否优雅关闭
    uint32_t shutdown_timeout_ms = 30000; // 关闭超时时间
    bool log_signal_events = true;     // 是否记录信号事件
};

/**
 * @brief 模拟Application类，演示增强信号处理
 */
class MockApplication {
public:
    MockApplication() : running_(false) {}
    
    ~MockApplication() {
        if (running_.load()) {
            Stop();
        }
    }
    
    /**
     * @brief 设置信号处理配置
     */
    void SetSignalHandlerConfig(const SignalHandlerConfig& config) {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        signal_config_ = config;
        
        if (running_.load()) {
            SetupSignalHandlers();
        }
    }
    
    /**
     * @brief 注册信号Hook（无返回值，总是继续默认处理）
     */
    void RegisterSignalHook(int signal, SignalHook hook) {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        signal_hooks_[signal].push_back(hook);
        
        if (signal_config_.log_signal_events) {
            std::cout << "📝 Registered signal hook for signal " << signal << std::endl;
        }
    }
    
    /**
     * @brief 注册信号Hook（Lambda版本）
     */
    template<typename Lambda>
    void RegisterSignalHook(int signal, Lambda&& lambda) {
        static_assert(std::is_invocable_v<Lambda, MockApplication&, int>, 
                     "Lambda must be callable with (MockApplication&, int)");
        RegisterSignalHook(signal, SignalHook(std::forward<Lambda>(lambda)));
    }
    
    /**
     * @brief 注册信号处理器（可决定是否继续默认处理）
     */
    void RegisterSignalHandler(int signal, SignalHandler handler) {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        signal_handlers_[signal].push_back(handler);
        
        if (signal_config_.log_signal_events) {
            std::cout << "📝 Registered signal handler for signal " << signal << std::endl;
        }
    }
    
    /**
     * @brief 注册信号处理器（Lambda版本）
     */
    template<typename Lambda>
    void RegisterSignalHandler(int signal, Lambda&& lambda) {
        static_assert(std::is_invocable_r_v<bool, Lambda, MockApplication&, int>, 
                     "Lambda must be callable with (MockApplication&, int) and return bool");
        RegisterSignalHandler(signal, SignalHandler(std::forward<Lambda>(lambda)));
    }
    
    /**
     * @brief 清除指定信号的所有处理器
     */
    void ClearSignalHandlers(int signal) {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        signal_hooks_[signal].clear();
        signal_handlers_[signal].clear();
        
        if (signal_config_.log_signal_events) {
            std::cout << "🗑️  Cleared signal handlers for signal " << signal << std::endl;
        }
    }
    
    /**
     * @brief 启动应用
     */
    bool Start() {
        if (running_.load()) {
            std::cout << "Application already running" << std::endl;
            return true;
        }
        
        std::cout << "🚀 Starting enhanced signal handling demo..." << std::endl;
        
        SetupSignalHandlers();
        running_.store(true);
        
        std::cout << "✅ Application started successfully" << std::endl;
        return true;
    }
    
    /**
     * @brief 运行应用
     */
    void Run() {
        if (!Start()) {
            std::cerr << "Failed to start application" << std::endl;
            return;
        }
        
        std::cout << "\n🎯 === Enhanced Signal Handling Demo ===\n";
        std::cout << "💡 Available signals:\n";
        std::cout << "  - SIGINT (Ctrl+C): Graceful shutdown with hooks\n";
        std::cout << "  - SIGTERM: Conditional shutdown with validation\n";
        std::cout << "  - SIGUSR1: Custom reload configuration\n";
        std::cout << "  - SIGUSR2: Custom status report\n";
        std::cout << "=========================================\n" << std::endl;
        
        // 模拟应用工作负载
        int counter = 0;
        while (running_.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            counter++;
            
            if (counter % 5 == 0) {
                std::cout << "⏰ Application working... (count: " << counter << ")" << std::endl;
            }
        }
        
        std::cout << "Application stopped" << std::endl;
    }
    
    /**
     * @brief 停止应用
     */
    void Stop() {
        if (!running_.load()) {
            return;
        }
        
        std::cout << "🛑 Stopping application..." << std::endl;
        running_.store(false);
        std::cout << "✅ Application stopped successfully" << std::endl;
    }
    
    /**
     * @brief 获取信号处理配置
     */
    const SignalHandlerConfig& GetSignalHandlerConfig() const { 
        return signal_config_; 
    }
    
    /**
     * @brief 检查应用是否运行
     */
    bool IsRunning() const { 
        return running_.load(); 
    }

private:
    /**
     * @brief 设置信号处理器
     */
    void SetupSignalHandlers() {
        // 注册系统信号处理器
        for (int signal : signal_config_.handled_signals) {
            std::signal(signal, &MockApplication::SystemSignalHandler);
            if (signal_config_.log_signal_events) {
                std::cout << "📡 Registered system signal handler for signal " << signal << std::endl;
            }
        }
    }
    
    /**
     * @brief 系统信号处理函数（静态）
     */
    static void SystemSignalHandler(int signal) {
        // 注意：这是一个简化的实现，实际应用中应该使用更安全的信号处理方式
        if (instance_) {
            instance_->OnSignalReceived(signal);
        }
    }
    
    /**
     * @brief 信号接收处理
     */
    void OnSignalReceived(int signal) {
        if (signal_config_.log_signal_events) {
            std::cout << "\n📨 Received signal " << signal;
        }
        
        bool continue_default_handling = true;
        
        // 根据策略处理信号
        switch (signal_config_.strategy) {
            case SignalHandlerStrategy::DEFAULT_ONLY:
                // 仅使用默认处理
                break;
                
            case SignalHandlerStrategy::HOOK_FIRST:
                // Hook优先，然后默认处理
                ProcessSignalHooks(signal);
                break;
                
            case SignalHandlerStrategy::HOOK_ONLY:
                // 仅使用Hook处理
                ProcessSignalHooks(signal);
                continue_default_handling = false;
                break;
                
            case SignalHandlerStrategy::HOOK_OVERRIDE:
                // Hook决定是否继续默认处理
                continue_default_handling = ProcessSignalHandlers(signal);
                break;
        }
        
        // 执行默认处理
        if (continue_default_handling) {
            ExecuteDefaultSignalHandler(signal);
        }
    }
    
    /**
     * @brief 处理信号Hooks
     */
    bool ProcessSignalHooks(int signal) {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        
        auto it = signal_hooks_.find(signal);
        if (it != signal_hooks_.end()) {
            for (auto& hook : it->second) {
                try {
                    hook(*this, signal);
                } catch (const std::exception& e) {
                    std::cerr << "❌ Signal hook error for signal " << signal << ": " << e.what() << std::endl;
                }
            }
        }
        
        return true; // Hook总是允许继续默认处理
    }
    
    /**
     * @brief 处理信号Handlers
     */
    bool ProcessSignalHandlers(int signal) {
        std::lock_guard<std::mutex> lock(signal_mutex_);
        
        auto it = signal_handlers_.find(signal);
        if (it != signal_handlers_.end()) {
            for (auto& handler : it->second) {
                try {
                    if (!handler(*this, signal)) {
                        return false; // 如果任何handler返回false，停止默认处理
                    }
                } catch (const std::exception& e) {
                    std::cerr << "❌ Signal handler error for signal " << signal << ": " << e.what() << std::endl;
                }
            }
        }
        
        return true; // 所有handler都返回true或没有handler
    }
    
    /**
     * @brief 执行默认信号处理
     */
    void ExecuteDefaultSignalHandler(int signal) {
        if (signal_config_.log_signal_events) {
            std::cout << ", executing default handler..." << std::endl;
        }
        
        // 默认处理逻辑
        if (signal == SIGINT || signal == SIGTERM) {
            if (signal_config_.graceful_shutdown) {
                std::cout << "🔄 Initiating graceful shutdown..." << std::endl;
                Stop();
            } else {
                std::cout << "⚡ Initiating immediate shutdown..." << std::endl;
                std::exit(signal == SIGINT ? 130 : 143);
            }
        } else {
            std::cout << "ℹ️  Received signal " << signal << " but no default handler defined" << std::endl;
        }
    }

private:
    // 信号处理相关
    SignalHandlerConfig signal_config_;
    std::unordered_map<int, std::vector<SignalHook>> signal_hooks_;
    std::unordered_map<int, std::vector<SignalHandler>> signal_handlers_;
    mutable std::mutex signal_mutex_;
    
    // 应用状态
    std::atomic<bool> running_;
    
    // 单例支持（简化版）
    static MockApplication* instance_;

public:
    static void SetInstance(MockApplication* app) { instance_ = app; }
};

// 静态成员定义
MockApplication* MockApplication::instance_ = nullptr;

/**
 * @brief 设置自定义信号处理
 */
void SetupCustomSignalHandling(MockApplication& app) {
    // 设置信号处理配置
    SignalHandlerConfig config;
    config.strategy = SignalHandlerStrategy::HOOK_FIRST; // Hook优先，然后默认处理
    config.handled_signals = {SIGINT, SIGTERM, SIGUSR1, SIGUSR2}; // 添加自定义信号
    config.graceful_shutdown = true;
    config.shutdown_timeout_ms = 15000; // 15秒超时
    config.log_signal_events = true;
    app.SetSignalHandlerConfig(config);
    
    // 注册SIGINT的Hook（在默认处理之前执行）
    app.RegisterSignalHook(SIGINT, [](MockApplication& app, int signal) {
        std::cout << "\n🔔 Custom SIGINT Hook: Saving temporary data..." << std::endl;
        // 这里可以执行清理操作
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 模拟清理时间
        std::cout << "✅ Temporary data saved" << std::endl;
    });
    
    // 注册SIGTERM的Handler（可以决定是否继续默认处理）
    app.RegisterSignalHandler(SIGTERM, [](MockApplication& app, int signal) -> bool {
        std::cout << "\n🛡️  Custom SIGTERM Handler: Checking if shutdown is allowed..." << std::endl;
        
        // 这里可以添加验证逻辑
        static int call_count = 0;
        call_count++;
        
        if (call_count == 1) {
            std::cout << "⚠️  First SIGTERM received, asking for confirmation..." << std::endl;
            std::cout << "💡 Send SIGTERM again to confirm shutdown" << std::endl;
            return false; // 返回false表示不执行默认关闭处理
        } else {
            std::cout << "✅ Shutdown confirmed" << std::endl;
            return true; // 返回true表示继续默认关闭处理
        }
    });
    
    // 注册SIGUSR1的自定义处理（用于重载配置）
    app.RegisterSignalHook(SIGUSR1, [](MockApplication& app, int signal) {
        std::cout << "\n🔄 Received SIGUSR1 signal, reloading configuration..." << std::endl;
        // 这里可以实现配置重载逻辑
        std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 模拟重载时间
        std::cout << "✅ Configuration reloaded successfully" << std::endl;
    });
    
    // 注册SIGUSR2的状态报告（Lambda演示）
    app.RegisterSignalHook(SIGUSR2, [](MockApplication& app, int signal) {
        std::cout << "\n📊 Received SIGUSR2 signal, generating status report..." << std::endl;
        std::cout << "📈 Application Status:" << std::endl;
        std::cout << "  - Running: " << (app.IsRunning() ? "Yes" : "No") << std::endl;
        std::cout << "  - Signal Strategy: ";
        
        const auto& config = app.GetSignalHandlerConfig();
        switch (config.strategy) {
            case SignalHandlerStrategy::DEFAULT_ONLY:
                std::cout << "DEFAULT_ONLY"; break;
            case SignalHandlerStrategy::HOOK_FIRST:
                std::cout << "HOOK_FIRST"; break;
            case SignalHandlerStrategy::HOOK_ONLY:
                std::cout << "HOOK_ONLY"; break;
            case SignalHandlerStrategy::HOOK_OVERRIDE:
                std::cout << "HOOK_OVERRIDE"; break;
        }
        std::cout << std::endl;
        std::cout << "  - Graceful Shutdown: " << (config.graceful_shutdown ? "Enabled" : "Disabled") << std::endl;
        std::cout << "  - Handled Signals: ";
        for (size_t i = 0; i < config.handled_signals.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << config.handled_signals[i];
        }
        std::cout << std::endl;
        std::cout << "✅ Status report completed" << std::endl;
    });
}

} // namespace enhanced_signal_demo

/**
 * @brief 主函数 - 演示增强信号处理功能
 */
int main() {
    try {
        std::cout << "🎬 === Zeus Enhanced Signal Handling Demo ===\n" << std::endl;
        
        enhanced_signal_demo::MockApplication app;
        enhanced_signal_demo::MockApplication::SetInstance(&app);
        
        // 设置自定义信号处理
        enhanced_signal_demo::SetupCustomSignalHandling(app);
        
        // 运行应用
        app.Run();
        
        std::cout << "\n🎉 Demo completed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Application error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Unknown application error" << std::endl;
        return 1;
    }
}