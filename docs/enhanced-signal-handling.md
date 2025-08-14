# Zeus Application 框架 - 增强信号处理机制

## 概述

Zeus Application框架提供了强大而灵活的信号处理机制，支持多种处理方式，允许服务自定义信号处理逻辑，同时保持框架的默认处理能力。

## 核心特性

### 🔧 信号处理策略

框架支持四种信号处理策略：

1. **DEFAULT_ONLY**: 仅使用默认处理
2. **HOOK_FIRST**: Hook优先，然后执行默认处理
3. **HOOK_ONLY**: 仅使用Hook处理，跳过默认处理
4. **HOOK_OVERRIDE**: Hook决定是否继续默认处理

### 🎯 处理器类型

#### 1. SignalHook
```cpp
using SignalHook = std::function<void(Application&, int signal)>;
```
- **特点**: 无返回值，总是允许继续默认处理
- **用途**: 在信号处理前后执行额外操作（如清理、日志记录）

#### 2. SignalHandler
```cpp
using SignalHandler = std::function<bool(Application&, int signal)>;
```
- **特点**: 返回bool值控制是否继续默认处理
- **用途**: 实现条件处理、验证逻辑、自定义处理流程

## 使用方法

### 基础配置

```cpp
#include "core/app/application.h"

auto& app = ZEUS_APP();

// 设置信号处理配置
SignalHandlerConfig config;
config.strategy = SignalHandlerStrategy::HOOK_FIRST;
config.handled_signals = {SIGINT, SIGTERM, SIGUSR1, SIGUSR2};
config.graceful_shutdown = true;
config.shutdown_timeout_ms = 15000;
config.log_signal_events = true;
app.SetSignalHandlerConfig(config);
```

### Hook方式注册

```cpp
// 方式1：注册Hook函数
app.RegisterSignalHook(SIGINT, [](Application& app, int signal) {
    std::cout << "Saving temporary data..." << std::endl;
    // 执行清理操作
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Data saved successfully" << std::endl;
});

// 方式2：注册Handler函数
app.RegisterSignalHandler(SIGTERM, [](Application& app, int signal) -> bool {
    std::cout << "Checking shutdown conditions..." << std::endl;
    
    // 实现条件检查逻辑
    static int call_count = 0;
    call_count++;
    
    if (call_count == 1) {
        std::cout << "First SIGTERM, send again to confirm" << std::endl;
        return false; // 阻止默认关闭处理
    }
    
    return true; // 允许默认关闭处理
});
```

### Lambda方式注册

```cpp
// 自动推断类型的Lambda注册
app.RegisterSignalHook(SIGUSR1, [](auto& app, int signal) {
    std::cout << "Reloading configuration..." << std::endl;
    // 配置重载逻辑
});

// 状态报告Hook
app.RegisterSignalHook(SIGUSR2, [](Application& app, int signal) {
    const auto& config = app.GetSignalHandlerConfig();
    std::cout << "Application Status Report:" << std::endl;
    std::cout << "  Running: " << app.IsRunning() << std::endl;
    std::cout << "  Graceful Shutdown: " << config.graceful_shutdown << std::endl;
});
```

### Handler函数方式

```cpp
// 定义独立的处理函数
bool CustomSIGINTHandler(Application& app, int signal) {
    std::cout << "Custom SIGINT handler called" << std::endl;
    
    // 执行自定义逻辑
    if (/* 某些条件 */) {
        return false; // 阻止默认处理
    }
    
    return true; // 继续默认处理
}

// 注册处理函数
app.RegisterSignalHandler(SIGINT, CustomSIGINTHandler);
```

## 处理策略详解

### 策略对比

| 策略 | Hook执行 | Handler执行 | 默认处理 | 使用场景 |
|------|----------|-------------|----------|----------|
| DEFAULT_ONLY | ❌ | ❌ | ✅ | 标准应用，无需自定义处理 |
| HOOK_FIRST | ✅ | ❌ | ✅ | 需要在默认处理前后执行额外操作 |
| HOOK_ONLY | ✅ | ❌ | ❌ | 完全自定义信号处理 |
| HOOK_OVERRIDE | ❌ | ✅ | 条件性 | 需要条件控制默认处理 |

### 执行流程

```
信号接收 → 策略判断 → Hook/Handler执行 → 默认处理（条件性）
    ↓
【HOOK_FIRST流程】
信号 → Hook执行 → 默认处理

【HOOK_OVERRIDE流程】  
信号 → Handler执行 → 根据返回值决定默认处理
```

## 实际应用场景

### 1. Gateway服务增强

```cpp
void SetupGatewaySignalHandling(Application& app) {
    // 配置信号处理策略
    SignalHandlerConfig config;
    config.strategy = SignalHandlerStrategy::HOOK_FIRST;
    config.handled_signals = {SIGINT, SIGTERM, SIGUSR1};
    config.graceful_shutdown = true;
    app.SetSignalHandlerConfig(config);
    
    // SIGINT: 保存临时数据
    app.RegisterSignalHook(SIGINT, [](Application& app, int signal) {
        if (g_gateway_server) {
            const auto& stats = g_gateway_server->GetStats();
            std::cout << "Active sessions: " << stats.active_sessions << std::endl;
        }
    });
    
    // SIGTERM: 条件关闭验证
    app.RegisterSignalHandler(SIGTERM, [](Application& app, int signal) -> bool {
        if (g_gateway_server) {
            const auto& stats = g_gateway_server->GetStats();
            if (stats.active_sessions > 100) {
                std::cout << "Too many active sessions, shutdown denied" << std::endl;
                return false;
            }
        }
        return true;
    });
    
    // SIGUSR1: 重载配置
    app.RegisterSignalHook(SIGUSR1, [](Application& app, int signal) {
        std::cout << "Reloading gateway configuration..." << std::endl;
        // 实现热重载逻辑
    });
}
```

### 2. 数据服务保护

```cpp
void SetupDataServiceSignalHandling(Application& app) {
    app.RegisterSignalHandler(SIGINT, [](Application& app, int signal) -> bool {
        std::cout << "Checking for uncommitted transactions..." << std::endl;
        
        // 检查未提交的事务
        if (HasUncommittedTransactions()) {
            std::cout << "Warning: Uncommitted transactions detected!" << std::endl;
            std::cout << "Send SIGINT again to force shutdown" << std::endl;
            
            static bool force_shutdown = false;
            if (!force_shutdown) {
                force_shutdown = true;
                return false; // 第一次阻止关闭
            }
        }
        
        return true; // 允许关闭
    });
}
```

### 3. 监控和诊断

```cpp
void SetupMonitoringSignalHandling(Application& app) {
    // SIGUSR1: 内存使用报告
    app.RegisterSignalHook(SIGUSR1, [](Application& app, int signal) {
        std::cout << "=== Memory Usage Report ===" << std::endl;
        // 输出内存使用情况
        PrintMemoryUsage();
    });
    
    // SIGUSR2: 性能统计
    app.RegisterSignalHook(SIGUSR2, [](Application& app, int signal) {
        std::cout << "=== Performance Statistics ===" << std::endl;
        // 输出性能统计
        PrintPerformanceStats();
    });
}
```

## 最佳实践

### 1. 信号安全性
- 在信号处理函数中避免使用非信号安全的函数
- 使用原子操作和无锁数据结构
- 避免在信号处理中分配内存

### 2. 错误处理
```cpp
app.RegisterSignalHook(SIGINT, [](Application& app, int signal) {
    try {
        // 信号处理逻辑
        PerformCleanup();
    } catch (const std::exception& e) {
        // 记录错误，但不抛出异常
        std::cerr << "Signal handler error: " << e.what() << std::endl;
    }
});
```

### 3. 超时控制
```cpp
SignalHandlerConfig config;
config.shutdown_timeout_ms = 30000; // 30秒超时
config.graceful_shutdown = true;
app.SetSignalHandlerConfig(config);
```

### 4. 日志记录
```cpp
SignalHandlerConfig config;
config.log_signal_events = true; // 启用信号事件日志
app.SetSignalHandlerConfig(config);
```

## API参考

### 主要接口

```cpp
class Application {
public:
    // 配置管理
    void SetSignalHandlerConfig(const SignalHandlerConfig& config);
    const SignalHandlerConfig& GetSignalHandlerConfig() const;
    
    // Hook注册
    void RegisterSignalHook(int signal, hooks::SignalHook hook);
    template<typename Lambda>
    void RegisterSignalHook(int signal, Lambda&& lambda);
    
    // Handler注册
    void RegisterSignalHandler(int signal, hooks::SignalHandler handler);
    template<typename Lambda>
    void RegisterSignalHandler(int signal, Lambda&& lambda);
    
    // 清理方法
    void ClearSignalHandlers(int signal);
};
```

### 配置结构

```cpp
struct SignalHandlerConfig {
    SignalHandlerStrategy strategy = SignalHandlerStrategy::DEFAULT_ONLY;
    std::vector<int> handled_signals = {SIGINT, SIGTERM};
    bool graceful_shutdown = true;
    uint32_t shutdown_timeout_ms = 30000;
    bool log_signal_events = true;
};
```

## 演示程序

运行演示程序查看完整功能：

```bash
# 编译演示程序
cd examples/enhanced_signal_handling
g++ -std=c++17 -Wall -Wextra -O2 -o enhanced_signal_demo enhanced_signal_demo.cpp

# 运行演示
./enhanced_signal_demo

# 测试不同信号
# Ctrl+C (SIGINT) - 优雅关闭
# kill -TERM <pid> - 条件关闭
# kill -USR1 <pid> - 重载配置  
# kill -USR2 <pid> - 状态报告
```

## 总结

Zeus Application框架的增强信号处理机制提供了：

- **灵活性**: 支持多种处理策略和注册方式
- **安全性**: 内置错误处理和超时保护
- **易用性**: Lambda、函数指针等多种注册方式
- **可扩展性**: 支持自定义信号和处理逻辑
- **兼容性**: 保持与现有代码的兼容性

这使得各种Zeus服务能够根据自己的需求灵活定制信号处理逻辑，同时享受框架提供的默认处理能力。