# Lua Binding Generator 测试

这个目录包含了 `lua_binding_generator` 工具的测试用例。

## 目录结构

```
tests/lua_binding_generator/
├── CMakeLists.txt              # 测试构建配置
├── README.md                   # 本文档
├── TEST_GUIDE.md              # 详细测试指南
├── simple_binding_test.cpp    # 简单绑定测试（已验证可工作）
├── test_bindings.hpp          # 测试用的 C++ 类定义
├── test_bindings_impl.cpp     # 测试类实现
└── test_lua_script.lua        # Lua 测试脚本
```

## 如何运行测试

### 1. 构建测试

```bash
# 配置构建（包含工具和测试）
cmake -B build-tests -DBUILD_TOOLS=ON -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug

# 构建测试
cmake --build build-tests --target test_lua_binding_simple
```

### 2. 运行测试

```bash
# 直接运行测试
./build-tests/tests/lua_binding_generator/test_lua_binding_simple

# 或者使用相对路径
cd build-tests/tests/lua_binding_generator && ./test_lua_binding_simple
```

### 3. 预期输出

```
=== Testing Low-Level Lua Bindings ===
Calculator bindings registered successfully!

--- Test 1: Basic Calculator Usage ---
Initial value:	0
After adding 10:	10
After subtracting 3:	7
After reset:	0

--- Test 2: Static Methods ---
6 * 7 =	42

--- Test 3: Multiple Calculator Instances ---
Calculator 1 value:	100
Calculator 2 value:	200
After calc1 reset:
Calculator 1 value:	0
Calculator 2 value:	200

--- Test 4: Error Handling ---
Error caught correctly:	[string "..."]:4: bad argument #1 to 'add' (number expected, got string)

--- Test 5: Performance Test ---
Created and operated 100 calculators in 0.000 seconds
Last calculator value:	104

=== All Tests Completed! ===
This demonstrates the basic patterns that lua_binding_generator would produce.
Sol2 would handle the boilerplate C wrapper functions automatically.
```

## 测试内容

### simple_binding_test.cpp
- **目的**: 验证 lua_binding_generator 生成的绑定模式的正确性
- **测试内容**: 
  - C++ 对象在 Lua 中的创建和销毁
  - 方法调用
  - 静态方法调用  
  - 错误处理
  - 多实例管理
  - 基本性能测试

### test_bindings.hpp
- **目的**: 包含各种 EXPORT_LUA 宏标记的 C++ 结构
- **测试内容**:
  - 类导出 (`EXPORT_LUA_CLASS`)
  - 方法导出 (`EXPORT_LUA_METHOD`) 
  - 属性导出 (`EXPORT_LUA_PROPERTY`)
  - 静态方法导出 (`EXPORT_LUA_STATIC_METHOD`)
  - 构造函数导出 (`EXPORT_LUA_CONSTRUCTOR`)
  - 枚举导出 (`EXPORT_LUA_ENUM`)
  - 全局函数导出 (`EXPORT_LUA_FUNCTION`)
  - 常量导出 (`EXPORT_LUA_CONSTANT`)

### test_lua_script.lua
- **目的**: 全面的 Lua 脚本测试
- **测试内容**: 涵盖所有绑定功能的使用场景

## 状态

- ✅ **简单绑定测试**: 已验证可工作
- ✅ **CMake 集成**: 已集成到项目构建系统
- ✅ **目录结构**: 遵循项目规范
- ⏳ **完整绑定测试**: 需要解决 Sol2 兼容性问题

## 相关工具

- **lua_binding_generator**: `./build-tools/tools/lua_binding_generator/lua_binding_generator`
- **示例代码**: `examples/lua_binding_generator/` 目录
- **模板文件**: `tools/lua_binding_generator/templates/` 目录

## 故障排除

如果测试失败，请检查：

1. **Lua 库是否正确链接**: 确保 `lua::lua` 目标可用
2. **头文件路径**: 确保 `include/common/lua/export_macros.h` 可访问
3. **构建配置**: 确保使用了 `-DBUILD_TOOLS=ON -DBUILD_TESTS=ON`

更多详细信息请参见 `TEST_GUIDE.md`。