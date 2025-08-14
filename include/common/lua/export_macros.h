/**
 * @file simplified_export_macros.h
 * @brief 极简化的 EXPORT_LUA 宏系统
 * 
 * 这个新的宏系统专注于最小化用户心理负担，通过智能推导减少重复的参数输入。
 * 大多数情况下，用户只需要使用无参数的宏，工具会自动从 AST 推导所需信息。
 */

#pragma once

/**
 * @brief 宏版本信息
 */
#define SIMPLIFIED_EXPORT_LUA_VERSION_MAJOR 2
#define SIMPLIFIED_EXPORT_LUA_VERSION_MINOR 0
#define SIMPLIFIED_EXPORT_LUA_VERSION_PATCH 0
#define SIMPLIFIED_EXPORT_LUA_VERSION "2.0.0"

/**
 * @brief 核心宏系统（15个核心宏 + 扩展宏）
 * 
 * 设计原则：
 * 1. 名称自动推导 - 从 AST 获取，无需重复输入
 * 2. 命名空间智能推导 - 优先级：明确指定 > C++ namespace > EXPORT_LUA_MODULE > 全局
 * 3. 参数可选 - 只在需要自定义时才填写
 * 4. 向后兼容 - 旧的完整参数形式仍然支持
 * 5. 全面覆盖 - 支持所有现代C++特性
 */

// ================================
// 1. 模块和命名空间宏
// ================================

/**
 * @brief 定义文件的默认 Lua 模块
 * @param ModuleName 模块名称
 * 
 * 用法：EXPORT_LUA_MODULE(GameCore)
 * 作用：文件中所有导出项默认归属到此模块，可被具体的 namespace 参数覆盖
 */
#define EXPORT_LUA_MODULE(ModuleName) \
    namespace { \
        static constexpr const char* __lua_module_name = #ModuleName; \
        __attribute__((annotate("lua_export_module:" #ModuleName))) \
        static const int __lua_module_marker = 0; \
    }

/**
 * @brief 导出整个 C++ 命名空间
 * @param ... 可选参数：alias=别名
 * 
 * 用法：
 * EXPORT_LUA_NAMESPACE()              // 自动推导命名空间名
 * EXPORT_LUA_NAMESPACE(alias=Game)    // 指定 Lua 中的别名
 */
#define EXPORT_LUA_NAMESPACE(...) \
    __attribute__((annotate("lua_export_namespace:auto:" #__VA_ARGS__)))

// ================================
// 2. 类和结构体宏
// ================================

/**
 * @brief 导出类及其所有公共成员
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * class EXPORT_LUA_CLASS() Player {};                    // 最简形式，自动推导所有信息
 * class EXPORT_LUA_CLASS(namespace=game) Player {};      // 指定命名空间
 * class EXPORT_LUA_CLASS(alias=GamePlayer) Player {};    // 指定 Lua 别名
 * class EXPORT_LUA_CLASS(namespace=game, alias=Hero) Player {};  // 组合使用
 * 
 * 自动导出内容：
 * - 所有公共构造函数
 * - 所有公共成员方法  
 * - 所有公共静态方法
 * - 自动识别 get/set 方法配对为属性
 * - 继承关系（如果有）
 */
#define EXPORT_LUA_CLASS(...) \
    __attribute__((annotate("lua_export_class:auto:" #__VA_ARGS__)))

/**
 * @brief 导出枚举及其所有值
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * enum class EXPORT_LUA_ENUM() PlayerStatus { ALIVE, DEAD };
 * enum class EXPORT_LUA_ENUM(namespace=game) Status { OK, ERROR };
 */
#define EXPORT_LUA_ENUM(...) \
    __attribute__((annotate("lua_export_enum:auto:" #__VA_ARGS__)))

/**
 * @brief 导出单例类
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * class EXPORT_LUA_SINGLETON() GameManager {
 * public:
 *     static GameManager& getInstance();  // 自动识别为单例访问方法
 *     // 其他公共方法自动导出
 * };
 * 
 * 自动功能：
 * - 自动识别单例访问方法（getInstance, instance, get等）
 * - 生成 Lua 单例访问模式
 * - 导出所有公共方法
 */
#define EXPORT_LUA_SINGLETON(...) \
    __attribute__((annotate("lua_export_singleton:auto:" #__VA_ARGS__)))

/**
 * @brief 导出静态类（只包含静态方法的类）
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * class EXPORT_LUA_STATIC_CLASS() MathUtils {
 * public:
 *     static double clamp(double value, double min, double max);  // 自动导出
 *     static int random(int min, int max);                       // 自动导出
 * };
 * 
 * 自动功能：
 * - 只导出静态方法和静态变量
 * - 忽略构造函数和析构函数
 * - 在 Lua 中创建表而不是类
 */
#define EXPORT_LUA_STATIC_CLASS(...) \
    __attribute__((annotate("lua_export_static_class:auto:" #__VA_ARGS__)))

/**
 * @brief 导出抽象基类（包含纯虚函数的类）
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * class EXPORT_LUA_ABSTRACT_CLASS() Component {
 * public:
 *     virtual void update() = 0;        // 纯虚函数，在Lua中不直接绑定
 *     virtual bool isActive() const;    // 普通虚函数，正常导出
 * };
 * 
 * 自动功能：
 * - 识别纯虚函数，不生成直接绑定
 * - 支持多态和继承
 * - 为派生类提供基类接口
 */
#define EXPORT_LUA_ABSTRACT_CLASS(...) \
    __attribute__((annotate("lua_export_abstract_class:auto:" #__VA_ARGS__)))

// ================================
// 3. 函数和变量宏
// ================================

/**
 * @brief 导出全局函数
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * EXPORT_LUA_FUNCTION()                           // 自动推导函数名
 * int calculateDamage(int attack, int defense);
 * 
 * EXPORT_LUA_FUNCTION(namespace=math, alias=distance)  // 自定义参数
 * double calculateDistance(double x1, double y1, double x2, double y2);
 */
#define EXPORT_LUA_FUNCTION(...) \
    __attribute__((annotate("lua_export_function:auto:" #__VA_ARGS__)))

/**
 * @brief 导出变量（全局变量或静态成员变量）
 * @param ... 可选参数：access=readonly|readwrite, namespace=命名空间, alias=别名
 * 
 * 用法：
 * EXPORT_LUA_VARIABLE()                               // 默认只读
 * static const int MAX_PLAYERS = 100;
 * 
 * EXPORT_LUA_VARIABLE(access=readwrite, namespace=config)  // 可读写
 * static int current_level = 1;
 */
#define EXPORT_LUA_VARIABLE(...) \
    __attribute__((annotate("lua_export_variable:auto:" #__VA_ARGS__)))

/**
 * @brief 导出常量（自动推导为只读）
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * EXPORT_LUA_CONSTANT()
 * static const int MAX_PLAYERS = 100;         // 自动推导为只读常量
 * 
 * EXPORT_LUA_CONSTANT(namespace=config, alias=maxPlayers)
 * static const double PI = 3.14159;           // 自定义命名空间和别名
 * 
 * 自动功能：
 * - 自动推导常量名称和类型
 * - 强制设置为只读访问
 * - 支持 const、constexpr、static const 等
 */
#define EXPORT_LUA_CONSTANT(...) \
    __attribute__((annotate("lua_export_constant:auto:" #__VA_ARGS__)))

// ================================
// 4. 特殊类型宏
// ================================

/**
 * @brief 导出 STL 容器及其标准方法
 * @param ContainerType STL 容器类型（自动从声明推导，此参数用于显式指定）
 * @param ... 可选参数：alias=别名, namespace=命名空间
 * 
 * 用法：
 * EXPORT_LUA_STL(std::vector<int>)                    // 显式指定类型
 * EXPORT_LUA_STL(std::map<string,Player*>, alias=PlayerRegistry)  // 带别名
 * 
 * 自动生成的方法（以 vector 为例）：
 * - 构造函数、size()、empty()、clear()
 * - push_back()、pop_back()、front()、back()
 * - at()、operator[]（作为 get/set 方法）
 * - begin()、end()（迭代器支持）
 * - Lua 友好的 #操作符支持
 */
#define EXPORT_LUA_STL(ContainerType, ...) \
    __attribute__((annotate("lua_export_stl:" #ContainerType ":" #__VA_ARGS__)))

/**
 * @brief 导出回调函数（std::function 类型）
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * EXPORT_LUA_CALLBACK()                               // 自动推导参数类型
 * std::function<void(Player*, int)> OnPlayerDamage;
 * 
 * EXPORT_LUA_CALLBACK(alias=damageHandler)            // 自定义别名
 * std::function<bool(Player*, ActionType)> OnValidateAction;
 * 
 * 自动功能：
 * - 从 std::function<RetType(Args...)> 自动推导参数类型和数量
 * - 支持无限数量的参数
 * - 生成类型安全的 Lua 回调绑定
 */
#define EXPORT_LUA_CALLBACK(...) \
    __attribute__((annotate("lua_export_callback:auto:" #__VA_ARGS__)))

// ================================
// 5. 运算符重载宏
// ================================

/**
 * @brief 导出运算符重载
 * @param op 运算符符号
 * @param ... 可选参数：alias=别名
 * 
 * 用法：
 * EXPORT_LUA_OPERATOR(+)
 * Vector2D operator+(const Vector2D& other) const;    // 自动导出为 __add 元方法
 * 
 * EXPORT_LUA_OPERATOR([], alias=get)
 * double operator[](int index) const;                  // 自定义别名
 * 
 * 支持的运算符：
 * - 算术运算符：+, -, *, /, %, +=, -=, *=, /=
 * - 比较运算符：==, !=, <, <=, >, >=
 * - 一元运算符：-, +, ++, --
 * - 下标运算符：[]
 * - 函数调用运算符：()
 * - 赋值运算符：=
 * - 流运算符：<<, >>
 * 
 * 自动功能：
 * - 自动映射到对应的 Lua 元方法
 * - 支持链式调用
 * - 类型安全检查
 */
#define EXPORT_LUA_OPERATOR(op, ...) \
    __attribute__((annotate("lua_export_operator:" #op ":" #__VA_ARGS__)))

// ================================
// 6. 模板和泛型宏
// ================================

/**
 * @brief 导出模板类
 * @param T 模板参数类型
 * @param ... 可选参数：namespace=命名空间, alias=别名
 * 
 * 用法：
 * template<typename T>
 * class EXPORT_LUA_TEMPLATE(T) Container {
 *     // 模板类内容
 * };
 * 
 * 注意：需要配合 EXPORT_LUA_TEMPLATE_INSTANCE 使用
 */
#define EXPORT_LUA_TEMPLATE(T, ...) \
    __attribute__((annotate("lua_export_template:" #T ":" #__VA_ARGS__)))

/**
 * @brief 导出模板类的特化实例
 * @param TemplateInstance 完整的模板实例类型
 * @param ... 可选参数：alias=别名
 * 
 * 用法：
 * EXPORT_LUA_TEMPLATE_INSTANCE(Container<int>)
 * EXPORT_LUA_TEMPLATE_INSTANCE(Container<std::string>, alias=StringContainer)
 */
#define EXPORT_LUA_TEMPLATE_INSTANCE(TemplateInstance, ...) \
    __attribute__((annotate("lua_export_template_instance:" #TemplateInstance ":" #__VA_ARGS__)))

// ================================
// 7. 控制宏
// ================================

/**
 * @brief 忽略特定成员的导出
 * @param ... 可选参数：reason=原因说明
 * 
 * 用法：
 * EXPORT_LUA_IGNORE()                     // 简单忽略
 * void internalMethod();
 * 
 * EXPORT_LUA_IGNORE(reason="internal use only")  // 带说明
 * void debugMethod();
 * 
 * 在使用 EXPORT_LUA_CLASS() 自动导出所有公共成员时，
 * 此宏可以排除特定不想导出的成员。
 */
#define EXPORT_LUA_IGNORE(...) \
    __attribute__((annotate("lua_export_ignore:auto:" #__VA_ARGS__)))

// ================================
// 8. 细粒度控制宏（可选使用）
// ================================

/**
 * @brief 单独控制属性导出（当不想使用类的自动导出时）
 * @param ... 可选参数：access=readonly|readwrite, getter=获取器, setter=设置器, alias=别名
 * 
 * 用法：
 * EXPORT_LUA_PROPERTY()                           // 自动推导为只读属性
 * int getHealth() const;
 * 
 * EXPORT_LUA_PROPERTY(access=readwrite, setter=setHealth)  // 读写属性
 * int getHealth() const;
 * void setHealth(int health);
 * 
 * EXPORT_LUA_PROPERTY(alias=hp, access=readonly)          // 自定义名称
 * int getHealth() const;
 */
#define EXPORT_LUA_PROPERTY(...) \
    __attribute__((annotate("lua_export_property:auto:" #__VA_ARGS__)))

// ================================
// 9. 便利宏和别名
// ================================

/**
 * @brief 只读属性的便利宏
 */
#define EXPORT_LUA_READONLY_PROPERTY(...) \
    EXPORT_LUA_PROPERTY(access=readonly, __VA_ARGS__)

/**
 * @brief 读写属性的便利宏
 */
#define EXPORT_LUA_READWRITE_PROPERTY(...) \
    EXPORT_LUA_PROPERTY(access=readwrite, __VA_ARGS__)

// ================================
// 10. 兼容性和扩展宏
// ================================

/**
 * @brief 为了保持向后兼容而保留的宏（不推荐新代码使用）
 */

// 保留旧的完整参数形式的宏，但不推荐使用
#define EXPORT_LUA_METHOD(MethodName, ...) \
    __attribute__((annotate("lua_export_method:" #MethodName ":" #__VA_ARGS__)))

#define EXPORT_LUA_STATIC_METHOD(MethodName, ...) \
    __attribute__((annotate("lua_export_static_method:" #MethodName ":" #__VA_ARGS__)))

#define EXPORT_LUA_CONSTRUCTOR(...) \
    __attribute__((annotate("lua_export_constructor:" #__VA_ARGS__)))

/**
 * @brief 条件编译支持
 * 
 * 在不支持属性注解的编译器上，这些宏会被定义为空，
 * 确保代码仍然可以正常编译
 */
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif

#if !__has_attribute(annotate) && !defined(__clang__)
// 如果编译器不支持 annotate 属性，将宏定义为空
#warning "Compiler does not support __attribute__((annotate)). EXPORT_LUA macros will have no effect."

#undef EXPORT_LUA_MODULE
#undef EXPORT_LUA_CLASS
#undef EXPORT_LUA_ENUM
#undef EXPORT_LUA_SINGLETON
#undef EXPORT_LUA_STATIC_CLASS
#undef EXPORT_LUA_ABSTRACT_CLASS
#undef EXPORT_LUA_FUNCTION
#undef EXPORT_LUA_VARIABLE
#undef EXPORT_LUA_CONSTANT
#undef EXPORT_LUA_STL
#undef EXPORT_LUA_CALLBACK
#undef EXPORT_LUA_OPERATOR
#undef EXPORT_LUA_TEMPLATE
#undef EXPORT_LUA_TEMPLATE_INSTANCE
#undef EXPORT_LUA_IGNORE
#undef EXPORT_LUA_PROPERTY
#undef EXPORT_LUA_NAMESPACE

#define EXPORT_LUA_MODULE(ModuleName)
#define EXPORT_LUA_CLASS(...)
#define EXPORT_LUA_ENUM(...)
#define EXPORT_LUA_SINGLETON(...)
#define EXPORT_LUA_STATIC_CLASS(...)
#define EXPORT_LUA_ABSTRACT_CLASS(...)
#define EXPORT_LUA_FUNCTION(...)
#define EXPORT_LUA_VARIABLE(...)
#define EXPORT_LUA_CONSTANT(...)
#define EXPORT_LUA_STL(ContainerType, ...)
#define EXPORT_LUA_CALLBACK(...)
#define EXPORT_LUA_OPERATOR(op, ...)
#define EXPORT_LUA_TEMPLATE(T, ...)
#define EXPORT_LUA_TEMPLATE_INSTANCE(TemplateInstance, ...)
#define EXPORT_LUA_IGNORE(...)
#define EXPORT_LUA_PROPERTY(...)
#define EXPORT_LUA_NAMESPACE(...)

#endif

/**
 * @brief 使用示例和最佳实践
 * 
 * 1. 最常见的用法（零配置）：
 * ```cpp
 * EXPORT_LUA_MODULE(GameCore)
 * 
 * // 普通类 - 自动导出所有公共成员
 * class EXPORT_LUA_CLASS() Player {
 * public:
 *     Player();
 *     std::string getName() const;        // 自动导出
 *     void setName(const std::string&);   // 自动导出
 *     int getLevel() const;               // 自动导出为只读属性
 *     
 *     EXPORT_LUA_IGNORE()                 // 不想导出的方法
 *     void internalMethod();
 * };
 * 
 * // 枚举 - 自动导出所有值
 * enum class EXPORT_LUA_ENUM() Status { ACTIVE, INACTIVE };
 * 
 * // 全局函数 - 自动推导函数名
 * EXPORT_LUA_FUNCTION()
 * int calculateDamage(int attack, int defense);
 * 
 * // 常量 - 自动推导为只读
 * EXPORT_LUA_CONSTANT()
 * static const int MAX_PLAYERS = 100;
 * ```
 * 
 * 2. 高级特性用法：
 * ```cpp
 * // 单例类
 * class EXPORT_LUA_SINGLETON() GameManager {
 * public:
 *     static GameManager& getInstance();
 *     void startGame();
 * };
 * 
 * // 静态工具类
 * class EXPORT_LUA_STATIC_CLASS() MathUtils {
 * public:
 *     static double clamp(double value, double min, double max);
 * };
 * 
 * // 抽象基类
 * class EXPORT_LUA_ABSTRACT_CLASS() Component {
 * public:
 *     virtual void update() = 0;
 *     virtual bool isActive() const;
 * };
 * 
 * // 运算符重载
 * class EXPORT_LUA_CLASS() Vector2D {
 * public:
 *     EXPORT_LUA_OPERATOR(+)
 *     Vector2D operator+(const Vector2D& other) const;
 *     
 *     EXPORT_LUA_OPERATOR([])
 *     double operator[](int index) const;
 * };
 * ```
 * 
 * 3. 容器和回调：
 * ```cpp
 * // STL 容器
 * EXPORT_LUA_STL(std::vector<Player*>, alias=PlayerList)
 * EXPORT_LUA_STL(std::map<std::string, int>, alias=Stats)
 * 
 * // 回调函数
 * class EXPORT_LUA_CLASS() EventSystem {
 * public:
 *     EXPORT_LUA_CALLBACK()
 *     std::function<void(Player*)> OnPlayerJoin;
 *     
 *     EXPORT_LUA_CALLBACK()
 *     std::function<bool(int, double)> OnValidateScore;
 * };
 * ```
 * 
 * 4. 自定义配置：
 * ```cpp
 * class EXPORT_LUA_CLASS(namespace=combat, alias=Warrior) Player {
 *     // 类的内容
 * };
 * 
 * EXPORT_LUA_FUNCTION(namespace=math, alias=distance)
 * double calculateDistance(double x1, double y1, double x2, double y2);
 * 
 * EXPORT_LUA_CONSTANT(namespace=config, alias=maxPlayers)
 * static const int MAX_PLAYERS = 100;
 * ```
 * 
 * 5. 模板类：
 * ```cpp
 * template<typename T>
 * class EXPORT_LUA_TEMPLATE(T) Container {
 *     // 模板类内容
 * };
 * 
 * // 实例化特定类型
 * EXPORT_LUA_TEMPLATE_INSTANCE(Container<int>, alias=IntContainer)
 * EXPORT_LUA_TEMPLATE_INSTANCE(Container<std::string>)
 * ```
 */