# Zeus Project CMake Find Modules

本目录包含了 Zeus 项目所需的第三方库的自定义 CMake Find 模块。

## 修复的问题

### 🔧 路径问题修复
- **FindGTest.cmake**: 修复了压缩包下载路径，现在正确下载到 `thirdparty/` 目录而不是根目录
- **FindSpdlog.cmake**: 修复了压缩包下载路径，现在正确下载到 `thirdparty/` 目录而不是根目录

### 📦 压缩包文件整理
已将错误位置的压缩包文件移动到正确位置：
- `googletest-1.17.0.tar.gz` → `thirdparty/googletest-1.17.0.tar.gz`
- `spdlog-1.15.3.tar.gz` → `thirdparty/spdlog-1.15.3.tar.gz`

## 现有的 Find 模块

| 模块名 | 描述 | 状态 |
|--------|------|------|
| **FindBoost.cmake** | Boost C++ 库 | ✅ 路径正确 |
| **FindClang.cmake** | Clang 编译器和 LibTooling | ✅ 路径正确 |
| **FindFmt.cmake** | fmt 格式化库 | ✅ 路径正确 |
| **FindGTest.cmake** | Google Test 测试框架 | ✅ 已修复路径 |
| **FindKcp.cmake** | KCP 网络协议库 | ✅ 路径正确 |
| **FindLLVM.cmake** | LLVM 编译器基础设施 | ✅ 路径正确 |
| **FindLua.cmake** | Lua 脚本语言 | ✅ 路径正确 |
| **FindNlohmannJson.cmake** | nlohmann/json JSON 库 | ✅ 路径正确 |
| **FindOpenSSL.cmake** | OpenSSL 加密库 | ✅ 路径正确 |
| **FindProtobuf.cmake** | Protocol Buffers | ✅ 路径正确 |
| **FindSol2.cmake** | Sol2 Lua 绑定库 | ✅ 路径正确 |
| **FindSpdlog.cmake** | spdlog 日志库 | ✅ 已修复路径 |

## 功能特性

每个 Find 模块都提供以下功能：

1. **自动下载**: 如果库不存在，自动从官方源下载
2. **自动解压**: 下载后自动解压到 `thirdparty/` 目录
3. **版本管理**: 指定精确的版本号，确保构建一致性
4. **目标创建**: 创建现代 CMake 目标（如 `fmt::fmt`, `GTest::GTest` 等）
5. **跨平台**: 支持 Windows、macOS、Linux

## 使用方法

在 CMakeLists.txt 中使用：

```cmake
find_package(Fmt REQUIRED)
target_link_libraries(your_target fmt::fmt)

find_package(GTest REQUIRED)
target_link_libraries(your_test GTest::GTest GTest::Main)

find_package(Spdlog REQUIRED)
target_link_libraries(your_target spdlog::spdlog)
```

## 目录结构

所有第三方库都安装在 `thirdparty/` 目录下：

```
zeus/
└── thirdparty/
    ├── boost-1.88.0/
    ├── boost-1.88.0.tar.gz
    ├── fmt-11.2.0/
    ├── fmt-11.2.0.tar.gz
    ├── googletest-1.17.0/
    ├── googletest-1.17.0.tar.gz          # ✅ 已修复位置
    ├── llvm-20.1.8/
    ├── llvm-20.1.8-build/
    ├── lua-5.4.8/
    ├── lua-5.4.8.tar.gz
    ├── sol2-3.3.0/
    ├── sol2-3.3.0.tar.gz
    ├── spdlog-1.15.3/
    ├── spdlog-1.15.3.tar.gz              # ✅ 已修复位置
    └── ...
```

## 故障排除

如果遇到第三方库问题：

1. **清理重新构建**:
   ```bash
   rm -rf build/
   cmake -B build
   ```

2. **手动删除库重新下载**:
   ```bash
   rm -rf thirdparty/library-name/
   rm -f thirdparty/library-name.tar.gz
   ```

3. **检查网络连接**: 确保可以访问 GitHub 等下载源

## 版本信息

- **Boost**: 1.88.0
- **fmt**: 11.2.0  
- **Google Test**: 1.17.0
- **LLVM/Clang**: 20.1.8
- **Lua**: 5.4.8
- **Sol2**: 3.3.0
- **spdlog**: 1.15.3
- **nlohmann/json**: 3.12.0
- **KCP**: 1.7