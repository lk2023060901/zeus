/**
 * @file comprehensive_test.h
 * @brief 全面的 Lua 绑定生成器测试示例
 * 
 * 包含所有 C++ 特性的完整测试用例
 */

#pragma once

#include "common/lua/export_macros.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <map>
#include <unordered_map>

// ================================
// 模块定义
// ================================

EXPORT_LUA_MODULE(ComprehensiveTest)

// ================================
// 命名空间测试
// ================================

namespace game {
namespace core {

/**
 * @brief 基础枚举 - 测试自动推导
 */
enum class EXPORT_LUA_ENUM() Status {
    ACTIVE,
    INACTIVE,
    PENDING,
    ERROR
};

/**
 * @brief 传统枚举 - 测试自动推导
 */
enum EXPORT_LUA_ENUM() Priority {
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    CRITICAL = 4
};

/**
 * @brief 常量定义 - 测试自动推导
 */
EXPORT_LUA_CONSTANT()
static const int MAX_PLAYERS = 100;

EXPORT_LUA_CONSTANT()
static const double PI = 3.14159265359;

EXPORT_LUA_CONSTANT()
static const std::string GAME_NAME = "TestGame";

/**
 * @brief 全局变量 - 可读写
 */
EXPORT_LUA_VARIABLE()
static int g_debug_level = 0;

EXPORT_LUA_VARIABLE()
static bool g_verbose_mode = false;

/**
 * @brief 全局函数 - 测试自动推导
 */
EXPORT_LUA_FUNCTION()
double calculateDistance(double x1, double y1, double x2, double y2);

EXPORT_LUA_FUNCTION()
std::string formatMessage(const std::string& msg, int level);

EXPORT_LUA_FUNCTION()
bool validateInput(const std::string& input);

/**
 * @brief 基类 - 测试继承
 */
class EXPORT_LUA_CLASS() Entity {
public:
    Entity();
    Entity(int id, const std::string& name);
    virtual ~Entity();
    
    // 基本属性 - 自动推导
    int getId() const;
    void setId(int id);
    
    std::string getName() const;
    void setName(const std::string& name);
    
    // 虚函数
    virtual void update(double deltaTime);
    virtual std::string toString() const;
    
    // 静态方法
    static int getNextId();
    static void resetIdCounter();

protected:
    int id_;
    std::string name_;
    static int next_id_;
};

/**
 * @brief 继承类 - 测试继承关系
 */
class EXPORT_LUA_CLASS() Player : public Entity {
public:
    Player();
    Player(int id, const std::string& name, int level);
    ~Player() override;
    
    // 重写虚函数
    void update(double deltaTime) override;
    std::string toString() const override;
    
    // 玩家特有属性
    int getLevel() const;
    void setLevel(int level);
    
    double getHealth() const;
    void setHealth(double health);
    
    double getMana() const;
    void setMana(double mana);
    
    // 智能指针方法
    std::shared_ptr<Entity> getTarget() const;
    void setTarget(std::shared_ptr<Entity> target);
    
    // STL 容器方法
    std::vector<std::string> getSkills() const;
    void addSkill(const std::string& skill);
    
    std::map<std::string, int> getInventory() const;
    void addItem(const std::string& item, int count);
    
    // 运算符重载
    Player& operator+=(int experience);
    bool operator==(const Player& other) const;
    bool operator<(const Player& other) const;

private:
    int level_;
    double health_;
    double mana_;
    std::shared_ptr<Entity> target_;
    std::vector<std::string> skills_;
    std::map<std::string, int> inventory_;
};

/**
 * @brief 单例类 - 测试单例模式
 */
class EXPORT_LUA_SINGLETON() GameManager {
public:
    // 单例访问
    static GameManager& getInstance();
    
    // 禁用拷贝
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    
    // 游戏管理方法
    void startGame();
    void stopGame();
    bool isGameRunning() const;
    
    // 玩家管理
    void addPlayer(std::shared_ptr<Player> player);
    void removePlayer(int playerId);
    std::shared_ptr<Player> getPlayer(int playerId) const;
    std::vector<std::shared_ptr<Player>> getAllPlayers() const;
    
    // 统计信息
    int getPlayerCount() const;
    double getGameTime() const;

private:
    GameManager();
    ~GameManager();
    
    bool game_running_;
    double game_time_;
    std::unordered_map<int, std::shared_ptr<Player>> players_;
};

/**
 * @brief 静态类 - 只包含静态方法
 */
class EXPORT_LUA_STATIC_CLASS() MathUtils {
public:
    // 数学工具函数
    static double clamp(double value, double min, double max);
    static double lerp(double a, double b, double t);
    static int random(int min, int max);
    static double randomFloat(double min, double max);
    
    // 向量运算
    static double dotProduct(double x1, double y1, double x2, double y2);
    static double magnitude(double x, double y);
    static void normalize(double& x, double& y);
    
    // 常量
    static const double PI;
    static const double E;
    static const double EPSILON;
};

/**
 * @brief 抽象基类 - 测试纯虚函数
 */
class EXPORT_LUA_ABSTRACT_CLASS() Component {
public:
    Component() = default;
    virtual ~Component() = default;
    
    // 纯虚函数
    virtual void initialize() = 0;
    virtual void update(double deltaTime) = 0;
    virtual void destroy() = 0;
    virtual std::string getTypeName() const = 0;
    
    // 普通虚函数
    virtual bool isActive() const;
    virtual void setActive(bool active);

protected:
    bool active_ = true;
};

/**
 * @brief 具体组件类
 */
class EXPORT_LUA_CLASS() TransformComponent : public Component {
public:
    TransformComponent();
    TransformComponent(double x, double y, double rotation);
    
    // 实现纯虚函数
    void initialize() override;
    void update(double deltaTime) override;
    void destroy() override;
    std::string getTypeName() const override;
    
    // 位置属性
    double getX() const;
    void setX(double x);
    
    double getY() const;
    void setY(double y);
    
    double getRotation() const;
    void setRotation(double rotation);
    
    // 便捷方法
    void translate(double dx, double dy);
    void rotate(double angle);

private:
    double x_, y_, rotation_;
};

} // namespace core

namespace events {

/**
 * @brief 事件系统 - 测试回调函数
 */
class EXPORT_LUA_CLASS() EventSystem {
public:
    EventSystem();
    ~EventSystem();
    
    // 回调函数类型 - 自动推导参数
    EXPORT_LUA_CALLBACK()
    std::function<void()> OnGameStart;
    
    EXPORT_LUA_CALLBACK()
    std::function<void()> OnGameEnd;
    
    EXPORT_LUA_CALLBACK()
    std::function<void(std::shared_ptr<core::Player>)> OnPlayerJoin;
    
    EXPORT_LUA_CALLBACK()
    std::function<void(std::shared_ptr<core::Player>)> OnPlayerLeave;
    
    EXPORT_LUA_CALLBACK()
    std::function<void(std::shared_ptr<core::Player>, int, int)> OnPlayerLevelUp;
    
    EXPORT_LUA_CALLBACK()
    std::function<bool(const std::string&, double)> OnValidateAction;
    
    // 事件触发方法
    void triggerGameStart();
    void triggerGameEnd();
    void triggerPlayerJoin(std::shared_ptr<core::Player> player);
    void triggerPlayerLeave(std::shared_ptr<core::Player> player);
    void triggerPlayerLevelUp(std::shared_ptr<core::Player> player, int oldLevel, int newLevel);
    bool validateAction(const std::string& action, double value);

private:
    bool initialized_;
};

} // namespace events

namespace containers {

/**
 * @brief 容器工具类 - 测试 STL 容器绑定
 */
class EXPORT_LUA_CLASS() ContainerUtils {
public:
    ContainerUtils();
    
    // 返回各种 STL 容器
    std::vector<int> getIntVector() const;
    std::vector<std::string> getStringVector() const;
    std::vector<std::shared_ptr<core::Player>> getPlayerVector() const;
    
    std::map<std::string, int> getStringIntMap() const;
    std::map<int, std::shared_ptr<core::Player>> getPlayerMap() const;
    
    std::unordered_map<std::string, double> getStringDoubleMap() const;
    
    // 接受容器参数
    void processIntVector(const std::vector<int>& vec);
    void processStringVector(const std::vector<std::string>& vec);
    void processPlayerVector(const std::vector<std::shared_ptr<core::Player>>& vec);
    
    void processStringIntMap(const std::map<std::string, int>& map);
    void processPlayerMap(const std::map<int, std::shared_ptr<core::Player>>& map);

private:
    std::vector<int> int_vector_;
    std::vector<std::string> string_vector_;
    std::vector<std::shared_ptr<core::Player>> player_vector_;
    std::map<std::string, int> string_int_map_;
    std::map<int, std::shared_ptr<core::Player>> player_map_;
    std::unordered_map<std::string, double> string_double_map_;
};

} // namespace containers

namespace smartptr {

/**
 * @brief 智能指针测试类
 */
class EXPORT_LUA_CLASS() SmartPointerDemo {
public:
    SmartPointerDemo();
    ~SmartPointerDemo();
    
    // shared_ptr 方法
    std::shared_ptr<core::Player> createPlayer(const std::string& name);
    std::shared_ptr<core::Entity> createEntity(int id, const std::string& name);
    void setCurrentPlayer(std::shared_ptr<core::Player> player);
    std::shared_ptr<core::Player> getCurrentPlayer() const;
    
    // unique_ptr 方法
    std::unique_ptr<core::TransformComponent> createTransform();
    std::unique_ptr<core::TransformComponent> createTransform(double x, double y);
    
    // weak_ptr 方法
    std::weak_ptr<core::Player> getPlayerRef(int id) const;
    bool isPlayerValid(std::weak_ptr<core::Player> player) const;
    
    // 容器 + 智能指针
    std::vector<std::shared_ptr<core::Player>> getAllPlayers() const;
    std::map<int, std::shared_ptr<core::Entity>> getEntityMap() const;

private:
    std::shared_ptr<core::Player> current_player_;
    std::vector<std::shared_ptr<core::Player>> players_;
    std::map<int, std::shared_ptr<core::Entity>> entities_;
};

} // namespace smartptr

} // namespace game

// ================================
// STL 容器类型导出
// ================================

// 基础容器类型
EXPORT_LUA_STL(std::vector<int>)
EXPORT_LUA_STL(std::vector<double>) 
EXPORT_LUA_STL(std::vector<std::string>)
EXPORT_LUA_STL(std::vector<bool>)

// 智能指针容器
EXPORT_LUA_STL(std::vector<std::shared_ptr<game::core::Player>>)
EXPORT_LUA_STL(std::vector<std::shared_ptr<game::core::Entity>>)

// Map 容器
EXPORT_LUA_STL(std::map<std::string, int>)
EXPORT_LUA_STL(std::map<std::string, double>)
EXPORT_LUA_STL(std::map<std::string, std::string>)
EXPORT_LUA_STL(std::map<int, std::shared_ptr<game::core::Player>>)

// Unordered Map 容器
EXPORT_LUA_STL(std::unordered_map<std::string, int>)
EXPORT_LUA_STL(std::unordered_map<std::string, double>)

// ================================
// 运算符重载测试
// ================================

namespace operators {

/**
 * @brief 向量类 - 测试运算符重载
 */
class EXPORT_LUA_CLASS() Vector2D {
public:
    Vector2D();
    Vector2D(double x, double y);
    Vector2D(const Vector2D& other);
    
    // 属性访问
    double getX() const;
    void setX(double x);
    
    double getY() const;
    void setY(double y);
    
    // 数学运算符
    EXPORT_LUA_OPERATOR(+)
    Vector2D operator+(const Vector2D& other) const;
    
    EXPORT_LUA_OPERATOR(-)
    Vector2D operator-(const Vector2D& other) const;
    
    EXPORT_LUA_OPERATOR(*)
    Vector2D operator*(double scalar) const;
    
    EXPORT_LUA_OPERATOR(/)
    Vector2D operator/(double scalar) const;
    
    // 复合赋值运算符
    EXPORT_LUA_OPERATOR(+=)
    Vector2D& operator+=(const Vector2D& other);
    
    EXPORT_LUA_OPERATOR(-=)
    Vector2D& operator-=(const Vector2D& other);
    
    EXPORT_LUA_OPERATOR(*=)
    Vector2D& operator*=(double scalar);
    
    // 比较运算符
    EXPORT_LUA_OPERATOR(==)
    bool operator==(const Vector2D& other) const;
    
    EXPORT_LUA_OPERATOR(!=)
    bool operator!=(const Vector2D& other) const;
    
    // 一元运算符
    EXPORT_LUA_OPERATOR(-)
    Vector2D operator-() const;
    
    // 下标运算符
    EXPORT_LUA_OPERATOR([])
    double operator[](int index) const;
    
    // 函数调用运算符
    EXPORT_LUA_OPERATOR(())
    double operator()() const;  // 返回长度
    
    // 工具方法
    double length() const;
    double lengthSquared() const;
    Vector2D normalized() const;
    double dot(const Vector2D& other) const;

private:
    double x_, y_;
};

} // namespace operators

// ================================
// 模板类测试（如果支持）
// ================================

namespace templates {

/**
 * @brief 简单模板类测试
 */
template<typename T>
class EXPORT_LUA_TEMPLATE(T) Container {
public:
    Container();
    explicit Container(const T& value);
    
    void setValue(const T& value);
    T getValue() const;
    
    void push(const T& item);
    T pop();
    
    size_t size() const;
    bool empty() const;

private:
    std::vector<T> items_;
    T default_value_;
};

// 特化实例
EXPORT_LUA_TEMPLATE_INSTANCE(Container<int>)
EXPORT_LUA_TEMPLATE_INSTANCE(Container<std::string>)
EXPORT_LUA_TEMPLATE_INSTANCE(Container<double>)

} // namespace templates

/**
 * @brief 使用说明
 * 
 * 这个示例展示了新版本 Lua Binding Generator 支持的所有特性：
 * 
 * 1. 自动推导特性：
 *    - 类名、方法名、属性名自动推导
 *    - 命名空间自动映射
 *    - 枚举值自动导出
 *    - 常量自动导出
 *    - 属性自动配对（get/set方法）
 * 
 * 2. 高级C++特性：
 *    - 继承关系自动处理
 *    - 虚函数和纯虚函数
 *    - 单例模式支持
 *    - 静态类支持
 *    - 智能指针自动转换
 *    - 运算符重载
 * 
 * 3. STL容器支持：
 *    - 自动生成容器绑定
 *    - 支持嵌套容器
 *    - 智能指针容器
 * 
 * 4. 回调函数：
 *    - 自动推导参数类型
 *    - 支持任意参数数量
 *    - 返回值类型推导
 * 
 * 使用方法：
 * ./lua_binding_generator --module-name=ComprehensiveTest examples/comprehensive_test.h
 */