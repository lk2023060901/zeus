#pragma once

#include "common/lua/export_macros.h"

EXPORT_LUA_MODULE(SimpleTest)

namespace simple_test {

class EXPORT_LUA_CLASS() Calculator {
public:
    Calculator() : value_(0) {}
    
    void add(int x) { value_ += x; }
    int getValue() const { return value_; }
    void reset() { value_ = 0; }
    
private:
    int value_;
};

EXPORT_LUA_FUNCTION()
int square(int x) { return x * x; }

} // namespace simple_test