#pragma once

#include "common/lua/export_macros.h"

EXPORT_LUA_MODULE(SimpleTest)

namespace simple_test {

// 简单的测试类
class EXPORT_LUA_CLASS(Calculator, namespace=simple_test) Calculator {
public:
    EXPORT_LUA_CONSTRUCTOR()
    Calculator() : value_(0) {}
    
    EXPORT_LUA_METHOD(add)
    void add(int x) { value_ += x; }
    
    EXPORT_LUA_METHOD(subtract)
    void subtract(int x) { value_ -= x; }
    
    EXPORT_LUA_METHOD(getValue)
    int getValue() const { return value_; }
    
    EXPORT_LUA_METHOD(reset)
    void reset() { value_ = 0; }
    
    EXPORT_LUA_STATIC_METHOD(multiply)
    static int multiply(int a, int b) { return a * b; }

private:
    int value_;
};

// 全局函数
EXPORT_LUA_FUNCTION(square, namespace=simple_test)
int square(int x) { return x * x; }

} // namespace simple_test