/**
 * @file example_v2_usage.h
 * @brief Lua Binding Generator v2.0 使用示例
 * 
 * 展示新版本的极简化宏系统和智能推导功能
 */

#pragma once

#include "common/lua/export_macros.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

// ================================
// 1. 模块定义 - 文件级别的模块归属
// ================================

EXPORT_LUA_MODULE(GameDemo)

// ================================
// 2. 最简形式 - 零配置，完全自动推导
// ================================

namespace game {

/**
 * @brief 游戏玩家类 - 演示最简单的类导出
 */
class EXPORT_LUA_CLASS() Player {
public:
    // 构造函数 - 自动导出
    Player();
    Player(const std::string& name, int level);
    
    // 方法 - 自动导出，自动推导 Lua 名称
    std::string getName() const;              // 自动推导为 "getName"
    void setName(const std::string& name);    // 自动推导为 "setName"
    
    // 属性 - 自动识别 get/set 配对，生成 Lua 属性
    int getLevel() const;                     // 自动推导为只读属性 "level"
    
    int getHealth() const;                    // 自动推导为读写属性 "health"
    void setHealth(int health);
    
    int getMana() const;                      // 自动推导为只读属性 "mana"
    
    // 静态方法 - 自动导出
    static int getMaxLevel();
    static std::shared_ptr<Player> createDefault();
    
    // 普通方法 - 自动导出
    void attack(const std::string& target);
    bool isAlive() const;
    
    // 不想导出的方法 - 明确标记忽略
    EXPORT_LUA_IGNORE(reason="internal use only")
    void debugMethod();

private:
    std::string name_;                        // 私有成员自动忽略
    int level_;
    int health_;
    int mana_;
};

/**
 * @brief 枚举类型 - 自动导出所有值
 */
enum class EXPORT_LUA_ENUM() PlayerStatus {
    ALIVE,
    DEAD,
    RESPAWNING,
    DISCONNECTED
};

/**
 * @brief 全局函数 - 自动推导函数名
 */
EXPORT_LUA_FUNCTION()
int calculateDamage(int attack, int defense);

EXPORT_LUA_FUNCTION()
double calculateDistance(double x1, double y1, double x2, double y2);

} // namespace game

// ================================
// 3. 自定义配置 - 需要特殊设置时使用参数
// ================================

namespace combat {

/**
 * @brief 武器类 - 演示自定义命名空间和别名
 */
class EXPORT_LUA_CLASS(namespace=combat, alias=GameWeapon) Weapon {
public:
    EXPORT_LUA_CONSTRUCTOR()
    Weapon(const std::string& name, int damage);
    
    // 自定义方法别名
    EXPORT_LUA_METHOD(alias=getDamageValue)
    int getDamage() const;
    
    // 自定义属性访问控制
    EXPORT_LUA_PROPERTY(access=readonly, alias=weaponName)
    std::string getName() const;

private:
    std::string name_;
    int damage_;
};

/**
 * @brief 自定义命名空间的函数
 */
EXPORT_LUA_FUNCTION(namespace=combat, alias=computeAttackDamage)
int calculateCombatDamage(const Weapon& weapon, int armor);

} // namespace combat

// ================================
// 4. STL 容器支持 - 自动生成所有标准方法
// ================================

// 导出 STL 容器类型
EXPORT_LUA_STL(std::vector<int>, alias=IntArray)
EXPORT_LUA_STL(std::vector<std::shared_ptr<game::Player>>, alias=PlayerList)
EXPORT_LUA_STL(std::map<std::string, int>, alias=StringIntMap)

namespace inventory {

/**
 * @brief 背包系统 - 演示容器的使用
 */
class EXPORT_LUA_CLASS(namespace=inventory) Backpack {
public:
    EXPORT_LUA_CONSTRUCTOR()
    Backpack();
    
    // 返回 STL 容器的方法 - 自动推导为对应的 Lua 类型
    std::vector<std::string> getItemNames() const;
    std::map<std::string, int> getItemCounts() const;
    
    // 普通方法
    void addItem(const std::string& item, int count = 1);
    bool removeItem(const std::string& item, int count = 1);
    int getItemCount(const std::string& item) const;

private:
    std::map<std::string, int> items_;
};

} // namespace inventory

// ================================
// 5. 回调函数支持 - 自动推导参数类型
// ================================

namespace events {

/**
 * @brief 事件系统 - 演示回调函数的导出
 */
class EXPORT_LUA_CLASS(namespace=events) EventManager {
public:
    EXPORT_LUA_CONSTRUCTOR()
    EventManager();
    
    // 回调函数 - 自动推导参数类型，支持无限参数
    EXPORT_LUA_CALLBACK()
    std::function<void()> OnGameStart;
    
    EXPORT_LUA_CALLBACK()
    std::function<void(game::Player*)> OnPlayerJoin;
    
    EXPORT_LUA_CALLBACK()
    std::function<void(game::Player*, int)> OnPlayerDamage;
    
    EXPORT_LUA_CALLBACK()
    std::function<bool(game::Player*, const std::string&)> OnPlayerAction;
    
    EXPORT_LUA_CALLBACK(alias=damageValidator)
    std::function<bool(int, const std::string&, double)> OnValidateDamage;
    
    // 注册和触发方法
    void registerCallbacks();
    void triggerEvents();

private:
    bool initialized_;
};

} // namespace events

// ================================
// 6. 智能指针和现代 C++ 支持
// ================================

namespace modern {

/**
 * @brief 现代 C++ 特性演示
 */
class EXPORT_LUA_CLASS(namespace=modern) SmartPointerDemo {
public:
    EXPORT_LUA_CONSTRUCTOR()
    SmartPointerDemo();
    
    // 智能指针方法 - 自动处理智能指针绑定
    std::shared_ptr<game::Player> createPlayer(const std::string& name);
    std::unique_ptr<combat::Weapon> createWeapon(const std::string& type);
    std::weak_ptr<game::Player> getPlayerRef(int id);
    
    // 现代 C++ 容器
    std::vector<std::shared_ptr<game::Player>> getAllPlayers();
    std::unordered_map<int, std::shared_ptr<game::Player>> getPlayerMap();

private:
    std::vector<std::shared_ptr<game::Player>> players_;
    std::unordered_map<int, std::shared_ptr<game::Player>> player_map_;
};

} // namespace modern

// ================================
// 7. 全局常量和变量
// ================================

// 只读常量
EXPORT_LUA_VARIABLE(access=readonly, namespace=config)
static const int MAX_PLAYERS = 100;

EXPORT_LUA_VARIABLE(access=readonly, namespace=config)
static const double PI = 3.14159;

EXPORT_LUA_VARIABLE(access=readonly, namespace=config)
static const std::string GAME_VERSION = "1.0.0";

// 可读写变量
EXPORT_LUA_VARIABLE(access=readwrite, namespace=runtime)
static int current_level = 1;

EXPORT_LUA_VARIABLE(access=readwrite, namespace=runtime, alias=debugMode)
static bool debug_enabled = false;

/**
 * @brief 使用说明
 * 
 * 新版本的 Lua Binding Generator 使用方法：
 * 
 * 1. 编译工具：
 *    mkdir build && cd build
 *    cmake .. && make
 * 
 * 2. 运行生成器：
 *    ./lua_binding_generator --module-name=GameDemo --output-dir=bindings example_v2_usage.h
 * 
 * 3. 增量编译：
 *    ./lua_binding_generator --incremental example_v2_usage.h
 * 
 * 4. 并行处理：
 *    ./lua_binding_generator --parallel --max-threads=4 src/*.h
 * 
 * 5. 详细输出：
 *    ./lua_binding_generator --verbose --stats example_v2_usage.h
 * 
 * 6. 使用配置文件：
 *    ./lua_binding_generator --config=lua_bindings_config.json src/*.h
 * 
 * 生成的绑定代码将包含：
 * - 自动推导的类、方法、属性绑定
 * - 智能识别的 get/set 属性配对
 * - STL 容器的完整方法绑定
 * - 回调函数的类型安全绑定
 * - 命名空间的层次化组织
 * - 现代 C++ 特性的完整支持
 */