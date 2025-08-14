#pragma once

#include "common/lua/export_macros.h"
#include <string>
#include <vector>
#include <memory>

// 测试模块定义
EXPORT_LUA_MODULE(TestModule)

namespace test_bindings {

// 简单的测试枚举
enum class EXPORT_LUA_ENUM(Color, namespace=test_bindings) Color {
    Red = 0,
    Green = 1,
    Blue = 2
};

// 测试基类
class EXPORT_LUA_CLASS(Vehicle, namespace=test_bindings) Vehicle {
public:
    EXPORT_LUA_CONSTRUCTOR()
    Vehicle() : speed_(0) {}
    
    EXPORT_LUA_CONSTRUCTOR(speed)
    explicit Vehicle(int speed) : speed_(speed) {}
    
    virtual ~Vehicle() = default;
    
    EXPORT_LUA_METHOD(getSpeed)
    int getSpeed() const { return speed_; }
    
    EXPORT_LUA_METHOD(setSpeed)
    void setSpeed(int speed) { speed_ = speed; }
    
    EXPORT_LUA_READONLY_PROPERTY(maxSpeed)
    int getMaxSpeed() const { return 200; }
    
    EXPORT_LUA_STATIC_METHOD(getVehicleCount)
    static int getVehicleCount() { return vehicle_count_; }
    
    EXPORT_LUA_METHOD(start)
    virtual void start() { running_ = true; }
    
    EXPORT_LUA_METHOD(stop)
    virtual void stop() { running_ = false; }
    
    EXPORT_LUA_READONLY_PROPERTY(isRunning)
    bool isRunning() const { return running_; }

protected:
    int speed_;
    bool running_ = false;
    static int vehicle_count_;
};

// 测试派生类
class EXPORT_LUA_CLASS(Car, namespace=test_bindings, inherit=Vehicle) Car : public Vehicle {
public:
    EXPORT_LUA_CONSTRUCTOR()
    Car() : Vehicle(), brand_("Unknown") {}
    
    EXPORT_LUA_CONSTRUCTOR(brand, speed)
    Car(const std::string& brand, int speed) : Vehicle(speed), brand_(brand) {}
    
    EXPORT_LUA_METHOD(getBrand)
    std::string getBrand() const { return brand_; }
    
    EXPORT_LUA_METHOD(setBrand)
    void setBrand(const std::string& brand) { brand_ = brand; }
    
    EXPORT_LUA_READWRITE_PROPERTY(color, getter=getColor, setter=setColor)
    Color getColor() const { return color_; }
    void setColor(Color color) { color_ = color; }
    
    EXPORT_LUA_METHOD(start)
    void start() override { 
        Vehicle::start();
        engine_running_ = true;
    }
    
    EXPORT_LUA_METHOD(stop)
    void stop() override {
        Vehicle::stop();
        engine_running_ = false;
    }
    
    EXPORT_LUA_READONLY_PROPERTY(engineRunning)
    bool isEngineRunning() const { return engine_running_; }
    
    EXPORT_LUA_METHOD(honk)
    std::string honk() const { return "Beep beep!"; }

private:
    std::string brand_;
    Color color_ = Color::Red;
    bool engine_running_ = false;
};

// 测试工具类
class EXPORT_LUA_CLASS(MathUtils, namespace=test_bindings) MathUtils {
public:
    EXPORT_LUA_STATIC_METHOD(add)
    static int add(int a, int b) { return a + b; }
    
    EXPORT_LUA_STATIC_METHOD(multiply)
    static double multiply(double a, double b) { return a * b; }
    
    EXPORT_LUA_STATIC_METHOD(calculateDistance)
    static double calculateDistance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return sqrt(dx * dx + dy * dy);
    }
};

// 全局函数测试
EXPORT_LUA_FUNCTION(createCar, namespace=test_bindings)
std::shared_ptr<Car> createCar(const std::string& brand, int speed);

EXPORT_LUA_FUNCTION(printMessage, namespace=test_bindings)
void printMessage(const std::string& message);

EXPORT_LUA_FUNCTION(getRandomNumber, namespace=test_bindings)
int getRandomNumber();

// 常量测试
EXPORT_LUA_CONSTANT(MAX_SPEED, namespace=test_bindings, value=300)
constexpr int MAX_SPEED = 300;

EXPORT_LUA_CONSTANT(PI, namespace=test_bindings, value=3.14159)
constexpr double PI = 3.14159;

} // namespace test_bindings