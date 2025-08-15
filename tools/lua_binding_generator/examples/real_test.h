/**
 * @file real_test.h
 * @brief 真实的 Lua 绑定生成器测试（使用真正的宏）
 */

#pragma once

#include "common/lua/export_macros.h"

// 定义模块
EXPORT_LUA_MODULE(RealTest)

// 简单枚举测试
enum class EXPORT_LUA_ENUM() Status {
    ACTIVE,
    INACTIVE,
    PENDING
};

// 简单类测试
class EXPORT_LUA_CLASS() SimplePlayer {
public:
    SimplePlayer();
    SimplePlayer(int id);
    ~SimplePlayer();
    
    // 基本方法
    int getId() const;
    void setId(int id);
    
    // 静态方法
    static int getMaxPlayers();
    static void setMaxPlayers(int max);
    
private:
    int id_;
    static int max_players_;
};

// 全局函数测试
EXPORT_LUA_FUNCTION()
int add(int a, int b);

EXPORT_LUA_FUNCTION()
double multiply(double a, double b);

// 常量测试
EXPORT_LUA_CONSTANT()
static const int MAX_LEVEL = 100;

EXPORT_LUA_CONSTANT()
static const double PI_VALUE = 3.14159;

// 变量测试
EXPORT_LUA_VARIABLE()
static int g_current_level = 1;

// 单例类测试
class EXPORT_LUA_SINGLETON() GameConfig {
public:
    static GameConfig& getInstance();
    
    int getDifficulty() const;
    void setDifficulty(int difficulty);
    
    bool isDebugMode() const;
    void setDebugMode(bool debug);
    
private:
    GameConfig();
    ~GameConfig();
    
    int difficulty_;
    bool debug_mode_;
};

// 静态类测试
class EXPORT_LUA_STATIC_CLASS() MathUtils {
public:
    static int clamp(int value, int min, int max);
    static double lerp(double a, double b, double t);
    static int random(int min, int max);
};

// 抽象基类测试
class EXPORT_LUA_ABSTRACT_CLASS() Component {
public:
    Component() = default;
    virtual ~Component() = default;
    
    // 纯虚函数
    virtual void initialize() = 0;
    virtual void update(double deltaTime) = 0;
    
    // 普通虚函数
    virtual bool isActive() const;
    virtual void setActive(bool active);
    
protected:
    bool active_ = true;
};

// 具体组件类
class EXPORT_LUA_CLASS() TransformComponent : public Component {
public:
    TransformComponent();
    TransformComponent(double x, double y);
    
    // 实现纯虚函数
    void initialize() override;
    void update(double deltaTime) override;
    
    // 位置相关
    double getX() const;
    void setX(double x);
    
    double getY() const;
    void setY(double y);
    
private:
    double x_, y_;
};