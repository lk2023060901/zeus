# Zeus 项目构建选项说明

## 概览

Zeus 项目提供了多个 CMake 构建选项来控制不同模块和功能的编译。所有选项都可以通过 `cmake` 命令的 `-D` 参数进行配置。

## 核心构建选项

### 库类型选项

- **BUILD_SHARED_LIBS** (默认: ON)
  - 控制是否构建共享库（动态库）
  - `ON`: 构建 .so/.dylib/.dll 动态库
  - `OFF`: 构建 .a/.lib 静态库

### 模块选项

- **BUILD_GATEWAY** (默认: ON)
  - 控制是否构建 Gateway 模块
  - Gateway 是 Zeus 的核心网关服务

- **BUILD_TESTS** (默认: OFF)
  - 控制是否构建测试程序
  - 包含单元测试、集成测试等

- **BUILD_EXAMPLES** (默认: OFF)
  - 控制是否构建示例程序
  - 包含各种使用示例和演示代码

### 开发工具选项

- **BUILD_TOOLS** (默认: OFF) 
  - **主要工具选项**：控制是否构建所有开发工具
  - 启用后会构建包括 Lua 绑定生成器在内的所有工具
  - 推荐开发者使用此选项

- **BUILD_LUA_BINDINGS** (默认: OFF)
  - **专用选项**：仅控制 Lua 绑定生成器的构建
  - 可以独立于 BUILD_TOOLS 使用
  - 需要 LLVM/Clang 依赖

### 协议选项

- **USE_KCP_PROTOCOL** (默认: ON)
  - 控制 Zeus 服务使用的网络协议
  - `ON`: 使用 KCP 协议（低延迟）
  - `OFF`: 使用 TCP 协议（可靠传输）

## 使用示例

### 基本构建（仅核心模块）
```bash
# Debug 版本（自动创建 build/debug 目录）
mkdir -p build/debug && cd build/debug
cmake ../..
make -j4

# Release 版本（自动创建 build/release 目录）
mkdir -p build/release && cd build/release  
cmake -DCMAKE_BUILD_TYPE=Release ../..
make -j4
```

### 开发者构建（包含所有工具和测试）
```bash
# Debug 版本
mkdir -p build/debug && cd build/debug
cmake -DBUILD_TOOLS=ON -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ../..
make -j4

# Release 版本
mkdir -p build/release && cd build/release
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TOOLS=ON -DBUILD_TESTS=ON -DBUILD_EXAMPLES=ON ../..
make -j4
```

### 生产环境构建（静态库，无调试工具）
```bash
mkdir -p build/release && cd build/release
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DBUILD_TOOLS=OFF -DBUILD_TESTS=OFF ../..
make -j4
```

### 仅构建 Lua 绑定生成器
```bash
mkdir -p build/debug && cd build/debug
cmake -DBUILD_LUA_BINDINGS=ON ../..
make -j4 lua_binding_generator
```

### 使用 TCP 协议替代 KCP
```bash
mkdir -p build/debug && cd build/debug
cmake -DUSE_KCP_PROTOCOL=OFF ../..
make -j4
```

## 自动目录管理

Zeus 项目现在支持自动目录管理，无需手动创建输出目录：

### 自动目录创建
- **Debug 构建**：自动创建 `build/debug` 目录，所有输出文件存放其中
- **Release 构建**：自动创建 `build/release` 目录，所有输出文件存放其中
- **其他构建类型**：支持 `MinSizeRel` 和 `RelWithDebInfo`，分别创建对应目录

### 输出目录结构
```
zeus/
├── build/
│   ├── debug/          # Debug 构建输出
│   │   ├── libzeus_*.so
│   │   ├── zeus_gateway_server
│   │   └── lua_binding_generator
│   └── release/        # Release 构建输出
│       ├── libzeus_*.so
│       ├── zeus_gateway_server
│       └── lua_binding_generator
```

### CMake 配置信息
运行 `cmake` 时会显示当前的输出目录配置：
```
=== Zeus Project Configuration ===
Build type: Release
Output directory: /path/to/zeus/build/release
Library type: SHARED
...
===================================
```

## 选项组合说明

### 工具构建逻辑
- 当 `BUILD_TOOLS=ON` 时，会构建所有可用的开发工具
- 当 `BUILD_LUA_BINDINGS=ON` 时，仅构建 Lua 绑定生成器
- 两个选项可以同时启用，不会冲突
- 当两个选项都为 `OFF` 时，不会构建任何工具，也不会处理 tools 目录

### 依赖关系
- Lua 绑定生成器需要 LLVM/Clang 依赖，会自动下载
- 如果构建失败，请检查网络连接或手动下载依赖
- 工具构建是可选的，不影响核心框架的使用

## 构建输出查看

运行 `cmake` 时会显示详细的配置信息：

```
=== Zeus Project Configuration ===
Build type: Release
Library type: SHARED
C++ Standard: 17
Build Tests: OFF
Build Examples: OFF
Build Gateway: ON
Build Tools: OFF
Build Lua Bindings: OFF
Network Protocol: KCP (Low-latency)
===================================
```

这样可以清楚地看到当前启用了哪些选项。