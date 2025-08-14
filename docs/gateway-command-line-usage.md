# Zeus Gateway 增强多协议命令行参数使用指南

## 概述

Zeus Gateway现在支持真正强大的多协议、多端口命令行配置，可以在不修改配置文件的情况下快速部署复杂的网关服务。

## 核心特性

### 🚀 多协议同时支持

Gateway支持同时监听多种协议：

- **TCP**: 可靠传输协议，适合通用网络代理
- **HTTP**: Web服务协议，支持HTTP代理和负载均衡
- **KCP**: 高性能UDP协议，低延迟游戏/实时应用场景
- **HTTPS**: 安全Web协议，支持SSL/TLS加密

### 🌐 动态配置能力

- **多监听端点**: 可同时监听多个地址和端口
- **动态后端**: 运行时添加/配置后端服务器集群
- **性能调优**: 实时调整连接数、超时等参数
- **配置覆盖**: 命令行参数优先级高于配置文件

## 支持的命令行参数

### 基本参数

```bash
# 显示帮助信息
./zeus-gateway --help
./zeus-gateway -h

# 显示版本信息
./zeus-gateway --version  
./zeus-gateway -v

# 指定配置文件
./zeus-gateway --config config/gateway/gateway.prod.json
./zeus-gateway -c config/gateway/gateway.prod.json
```

### 多协议监听配置

```bash
# 单协议监听
./zeus-gateway --listen tcp://0.0.0.0:8080
./zeus-gateway --listen http://127.0.0.1:8081
./zeus-gateway --listen kcp://0.0.0.0:9000
./zeus-gateway --listen https://0.0.0.0:8443

# 多协议同时监听 (可重复指定)
./zeus-gateway \
  --listen tcp://0.0.0.0:8080 \
  --listen http://0.0.0.0:8081 \
  --listen kcp://0.0.0.0:9000 \
  --listen https://0.0.0.0:8443

# 简化格式 (默认协议为tcp)
./zeus-gateway --listen 0.0.0.0:8080    # 等同于 tcp://0.0.0.0:8080
```

### 动态后端服务器配置

```bash
# 单个后端服务器
./zeus-gateway --backend 192.168.1.10:8080

# 多个后端服务器 (可重复指定)
./zeus-gateway \
  --backend 192.168.1.10:8080 \
  --backend 192.168.1.11:8080 \
  --backend 192.168.1.12:8080

# 混合使用配置文件和命令行
./zeus-gateway -c base-config.json \
  --backend 192.168.1.20:8080 \
  --backend 192.168.1.21:8080
```

### 性能调优参数

```bash
# 设置最大连接数
./zeus-gateway --max-connections 5000

# 设置连接超时 (毫秒)
./zeus-gateway --timeout 30000

# 设置日志级别
./zeus-gateway --log-level debug
./zeus-gateway -l info

# 后台运行模式
./zeus-gateway --daemon
./zeus-gateway -d
```

### 实际部署场景

#### 场景1: 多协议游戏网关
```bash
./zeus-gateway \
  --listen tcp://0.0.0.0:8080 \
  --listen kcp://0.0.0.0:9000 \
  --listen http://0.0.0.0:8081 \
  --backend game-server-1:8080 \
  --backend game-server-2:8080 \
  --backend game-server-3:8080 \
  --max-connections 10000 \
  --timeout 5000 \
  --log-level info \
  --daemon
```

#### 场景2: Web负载均衡器
```bash
./zeus-gateway \
  --listen http://0.0.0.0:80 \
  --listen https://0.0.0.0:443 \
  --backend web-server-1:8080 \
  --backend web-server-2:8080 \
  --backend web-server-3:8080 \
  --max-connections 50000 \
  --timeout 30000 \
  --daemon
```

#### 场景3: 开发调试环境
```bash
./zeus-gateway \
  --listen tcp://127.0.0.1:8080 \
  --listen http://127.0.0.1:8081 \
  --backend localhost:3000 \
  --backend localhost:3001 \
  --log-level debug
```

#### 场景4: 容器化快速部署
```bash
# Docker环境中，使用环境变量传递参数
./zeus-gateway \
  --listen tcp://0.0.0.0:${GATEWAY_TCP_PORT:-8080} \
  --listen http://0.0.0.0:${GATEWAY_HTTP_PORT:-8081} \
  --backend ${BACKEND_1:-backend-1:8080} \
  --backend ${BACKEND_2:-backend-2:8080} \
  --max-connections ${MAX_CONN:-5000} \
  --daemon
```

## 参数验证

### 监听地址验证
```bash
# ✅ 有效格式
./zeus-gateway --listen tcp://0.0.0.0:8080
./zeus-gateway --listen http://127.0.0.1:8081
./zeus-gateway --listen kcp://0.0.0.0:9000
./zeus-gateway --listen 0.0.0.0:8080              # 默认tcp协议

# ❌ 无效格式 (会显示错误)
./zeus-gateway --listen invalid-format
./zeus-gateway --listen tcp://0.0.0.0:99999       # 端口超出范围
./zeus-gateway --listen unknown://0.0.0.0:8080    # 不支持的协议
```

### 后端服务器验证
```bash
# ✅ 有效格式
./zeus-gateway --backend 192.168.1.10:8080
./zeus-gateway --backend localhost:3000
./zeus-gateway --backend server.example.com:8080

# ❌ 无效格式 (会显示错误)
./zeus-gateway --backend invalid-format
./zeus-gateway --backend 192.168.1.10:99999       # 端口超出范围
./zeus-gateway --backend 192.168.1.10             # 缺少端口
```

### 性能参数验证
```bash
# ✅ 有效值
./zeus-gateway --max-connections 5000
./zeus-gateway --timeout 30000
./zeus-gateway --log-level info

# ❌ 无效值 (会显示错误)
./zeus-gateway --max-connections 0                 # 连接数必须大于0
./zeus-gateway --timeout abc                       # 无效的数字格式
./zeus-gateway --log-level invalid                 # 不支持的日志级别
```

## 配置覆盖优先级

配置值的优先级从高到低：

1. **命令行参数** (最高优先级)
2. **配置文件**
3. **默认值** (最低优先级)

### 示例

假设配置文件`gateway.json`中设置：
```json
{
  "gateway": {
    "listen": {
      "port": 8080,
      "bind_address": "0.0.0.0"
    }
  }
}
```

运行命令：
```bash
./zeus-gateway -c gateway.json -p 9090 -b 127.0.0.1
```

最终生效的配置：
- 端口: `9090` (命令行覆盖)
- 绑定地址: `127.0.0.1` (命令行覆盖)
- 其他配置: 来自配置文件

## 自定义Help和Version显示

### Help输出示例

```
🌉 Zeus Gateway Server v1.0.0 (Enhanced Multi-Protocol)
用法: ./zeus-gateway [选项]

选项:
  -c, --config <文件>           指定配置文件路径
      --listen <地址>           添加监听地址 (可多次指定)
                                格式: [protocol://]address:port
                                协议: tcp, kcp, http, https (默认: tcp)
      --backend <地址>          添加后端服务器 (可多次指定)
                                格式: address:port
      --max-connections <数量>  设置最大客户端连接数
      --timeout <毫秒>          设置连接超时时间
  -d, --daemon                  后台运行模式
  -l, --log-level <级别>        设置日志级别 (debug|info|warn|error)
  -h, --help                    显示此帮助信息
  -v, --version                 显示版本信息

使用示例:
  # 基本使用
  ./zeus-gateway                                  # 使用默认配置
  ./zeus-gateway -c config/prod.json               # 使用生产配置

  # 多协议监听
  ./zeus-gateway --listen tcp://0.0.0.0:8080 --listen http://0.0.0.0:8081
  ./zeus-gateway --listen kcp://0.0.0.0:9000 --listen https://0.0.0.0:8443

  # 动态后端配置
  ./zeus-gateway --backend 192.168.1.10:8080 --backend 192.168.1.11:8080

  # 完整配置示例
  ./zeus-gateway --listen tcp://0.0.0.0:8080 \
                 --backend 192.168.1.10:8080 \
                 --backend 192.168.1.11:8080 \
                 --max-connections 5000 \
                 --timeout 30000 --daemon -l info

信号处理:
  SIGINT (Ctrl+C)    - 优雅关闭
  SIGTERM            - 终止服务
  SIGUSR1            - 重载配置
  SIGUSR2            - 显示统计信息
```

### Version输出示例

```
🌉 Zeus Gateway Server
版本: 1.0.0 (Enhanced Multi-Protocol Framework)
构建时间: Aug 14 2025 02:30:15
框架: Zeus Application Framework v2.0

支持的协议:
  🌐 TCP  - 可靠传输协议
  🌐 HTTP - Web服务协议
  🚀 KCP  - 高性能UDP协议 (编译时禁用)
  🔒 HTTPS- 安全Web协议

功能特性:
  ✅ 多协议同时监听
  ✅ 动态后端服务器配置
  ✅ 增强命令行参数解析
  ✅ 灵活信号处理机制
  ✅ 协议路由和负载均衡
  ✅ 会话管理和实时统计
  ✅ 配置文件热重载
```

## 运行时参数显示

启动时会显示解析到的配置：

```
=== Zeus Gateway Server (Enhanced Multi-Protocol Framework) ===
📋 启动配置:
  配置文件: config/gateway/gateway.json.default
  监听地址:
    tcp://0.0.0.0:8080
    http://127.0.0.1:8081
    kcp://0.0.0.0:9000
  后端服务器:
    192.168.1.10:8080
    192.168.1.11:8080
    192.168.1.12:8080
  最大连接数: 5000
  超时时间: 30000ms
  日志级别: info
  运行模式: 后台运行

🚀 初始化Zeus Gateway服务...
网络协议: TCP (可靠)
⚠️  使用默认Gateway配置
📝 命令行覆盖 - 监听端点:
  添加监听: tcp://0.0.0.0:8080
  添加监听: http://127.0.0.1:8081
  添加监听: kcp://0.0.0.0:9000
📝 命令行覆盖 - 后端服务器:
  添加后端: 192.168.1.10:8080
  添加后端: 192.168.1.11:8080
  添加后端: 192.168.1.12:8080
📝 命令行覆盖 - 最大连接数: 5000
📝 命令行覆盖 - 超时时间: 30000ms
📝 后台运行模式已启用
✅ Gateway服务初始化成功
```

## 错误处理

### 参数解析错误

```bash
# 未知参数
$ ./zeus-gateway --unknown-option
❌ 参数解析错误: Unknown argument: --unknown-option

# 监听地址格式错误
$ ./zeus-gateway --listen invalid-format
❌ 错误: 无效的监听地址格式: invalid-format
格式: [protocol://]address:port
协议: tcp, kcp, http, https (默认: tcp)
示例: tcp://0.0.0.0:8080, http://127.0.0.1:8081, kcp://0.0.0.0:9000

# 不支持的协议
$ ./zeus-gateway --listen unknown://0.0.0.0:8080
❌ 错误: 无效的监听地址格式: unknown://0.0.0.0:8080

# 端口超出范围
$ ./zeus-gateway --listen tcp://0.0.0.0:99999
❌ 错误: 无效的监听地址格式: tcp://0.0.0.0:99999

# 后端服务器格式错误
$ ./zeus-gateway --backend invalid-format
❌ 错误: 后端地址格式无效: invalid-format
格式: address:port

# 性能参数错误
$ ./zeus-gateway --max-connections 0
❌ 错误: 最大连接数必须大于0

$ ./zeus-gateway --timeout abc
❌ 错误: 无效的超时时间: abc
```

### 自动帮助提示

参数解析失败时会自动显示帮助信息，帮助用户正确使用。

## 实现原理

### 参数定义
```cpp
void SetupGatewayArguments(Application& app) {
    // 配置文件参数
    app.AddArgument("c", "config", "指定配置文件路径", 
                   true, "config/gateway/gateway.json.default");
    
    // 端口参数 - 带验证处理器
    app.AddArgumentWithHandler("p", "port", "指定监听端口", 
        [](Application& app, const std::string& name, const std::string& value) -> bool {
            // 端口验证逻辑
            int port = std::stoi(value);
            return port > 0 && port <= 65535;
        }, true);
}
```

### 参数解析
```cpp
int main(int argc, char* argv[]) {
    auto& app = ZEUS_APP();
    
    // 设置参数定义
    SetupGatewayArguments(app);
    
    // 解析参数
    auto parsed_args = app.ParseArgs(argc, argv);
    
    // 处理结果
    if (parsed_args.help_requested) {
        app.ShowUsage(argv[0]);
        return 0;
    }
}
```

### 配置覆盖
```cpp
bool GatewayInitHook(Application& app) {
    // 加载配置文件
    // ...
    
    // 命令行参数覆盖
    if (app.HasArgument("port")) {
        gateway_config.listen_port = std::stoi(app.GetArgumentValue("port"));
    }
    
    if (app.HasArgument("bind")) {
        gateway_config.bind_address = app.GetArgumentValue("bind");
    }
}
```

## 总结

重构后的Zeus Gateway命令行系统提供了真正强大的功能：

### 🚀 核心优势

- **多协议支持**: 同时监听TCP、HTTP、KCP、HTTPS等多种协议
- **动态配置**: 运行时添加监听端点和后端服务器，无需重启
- **替代能力**: 命令行参数可以完全替代简单的配置文件
- **生产就绪**: 支持容器化部署、环境变量配置等现代部署方式

### 🎯 实际价值

- **快速部署**: 一条命令即可启动复杂的多协议网关
- **调试友好**: 开发环境中快速调整配置无需修改文件
- **运维简化**: 生产环境中灵活调整后端服务器和性能参数
- **容器化友好**: 完美支持Docker、Kubernetes等容器编排

### 📊 使用场景对比

| 场景 | 传统方式 | 新方式 | 优势 |
|------|----------|--------|------|
| 快速测试 | 修改配置文件 | `--listen tcp://127.0.0.1:8080 --backend localhost:3000` | 无需文件修改 |
| 多协议网关 | 复杂配置文件 | `--listen tcp://0.0.0.0:8080 --listen http://0.0.0.0:8081 --listen kcp://0.0.0.0:9000` | 一条命令完成 |
| 生产部署 | 多套配置文件 | 基础配置+命令行覆盖 | 配置复用性高 |
| 容器化 | 挂载配置卷 | 环境变量+命令行参数 | 云原生友好 |

这样的设计让Zeus Gateway从一个需要复杂配置的服务，变成了可以灵活部署、快速调试的现代化网关工具。