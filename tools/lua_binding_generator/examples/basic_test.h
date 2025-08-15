/**
 * @file basic_test.h
 * @brief 基础 Lua 绑定生成器测试
 */

#pragma once

// 模拟 export_macros.h 的基础宏定义
#define EXPORT_LUA_MODULE(name)
#define EXPORT_LUA_CLASS(...) [[annotate("lua_export_class")]]
#define EXPORT_LUA_FUNCTION(...) [[annotate("lua_export_function")]]
#define EXPORT_LUA_ENUM(...) [[annotate("lua_export_enum")]]
#define EXPORT_LUA_CONSTANT(...) [[annotate("lua_export_constant")]]

// 避免使用标准库头文件，简化测试
EXPORT_LUA_MODULE(BasicTest)

/**
 * @brief 基础枚举测试
 */
enum class EXPORT_LUA_ENUM() Status {
    ACTIVE,
    INACTIVE,
    PENDING
};

/**
 * @brief 基础类测试
 */
class EXPORT_LUA_CLASS() SimpleClass {
public:
    SimpleClass();
    SimpleClass(int value);
    ~SimpleClass();
    
    int getValue() const;
    void setValue(int value);
    
    void doSomething();
    
    static int getDefaultValue();

private:
    int value_;
};

/**
 * @brief 全局函数测试
 */
EXPORT_LUA_FUNCTION()
int add(int a, int b);

EXPORT_LUA_FUNCTION()
double multiply(double a, double b);

/**
 * @brief 常量测试
 */
EXPORT_LUA_CONSTANT()
static const int MAX_VALUE = 100;

EXPORT_LUA_CONSTANT()
static const double PI_VALUE = 3.14159;