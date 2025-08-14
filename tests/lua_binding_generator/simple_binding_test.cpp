/**
 * @file simple_binding_test.cpp
 * @brief Simple test to verify lua_binding_generator concepts work
 * 
 * This demonstrates the basic binding patterns that lua_binding_generator
 * would generate, using minimal Sol2 features to avoid compatibility issues.
 */

#include <lua.hpp>  // Use system Lua
#include <iostream>
#include <memory>
#include <string>

// Simple test class without complex inheritance for now
class Calculator {
public:
    Calculator() : value_(0) {}
    
    void add(int x) { value_ += x; }
    void subtract(int x) { value_ -= x; }
    int getValue() const { return value_; }
    void reset() { value_ = 0; }
    static int multiply(int a, int b) { return a * b; }
    
private:
    int value_;
};

// C-style wrapper functions for Lua binding (what Sol2 would generate internally)
extern "C" {
    static int calculator_new(lua_State* L) {
        Calculator** userdata = (Calculator**)lua_newuserdata(L, sizeof(Calculator*));
        *userdata = new Calculator();
        
        luaL_getmetatable(L, "Calculator");
        lua_setmetatable(L, -2);
        return 1;
    }
    
    static int calculator_add(lua_State* L) {
        Calculator** userdata = (Calculator**)luaL_checkudata(L, 1, "Calculator");
        int x = luaL_checkinteger(L, 2);
        (*userdata)->add(x);
        return 0;
    }
    
    static int calculator_subtract(lua_State* L) {
        Calculator** userdata = (Calculator**)luaL_checkudata(L, 1, "Calculator");
        int x = luaL_checkinteger(L, 2);
        (*userdata)->subtract(x);
        return 0;
    }
    
    static int calculator_getValue(lua_State* L) {
        Calculator** userdata = (Calculator**)luaL_checkudata(L, 1, "Calculator");
        lua_pushinteger(L, (*userdata)->getValue());
        return 1;
    }
    
    static int calculator_reset(lua_State* L) {
        Calculator** userdata = (Calculator**)luaL_checkudata(L, 1, "Calculator");
        (*userdata)->reset();
        return 0;
    }
    
    static int calculator_multiply(lua_State* L) {
        int a = luaL_checkinteger(L, 1);
        int b = luaL_checkinteger(L, 2);
        lua_pushinteger(L, Calculator::multiply(a, b));
        return 1;
    }
    
    static int calculator_gc(lua_State* L) {
        Calculator** userdata = (Calculator**)luaL_checkudata(L, 1, "Calculator");
        delete *userdata;
        return 0;
    }
}

// Register the Calculator class with Lua (similar to what Sol2 does)
void register_calculator(lua_State* L) {
    // Create metatable for Calculator
    luaL_newmetatable(L, "Calculator");
    
    // Set __gc metamethod for cleanup
    lua_pushstring(L, "__gc");
    lua_pushcfunction(L, calculator_gc);
    lua_settable(L, -3);
    
    // Set __index to point to the metatable itself
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);  // Push the metatable
    lua_settable(L, -3);
    
    // Register methods
    lua_pushstring(L, "add");
    lua_pushcfunction(L, calculator_add);
    lua_settable(L, -3);
    
    lua_pushstring(L, "subtract");
    lua_pushcfunction(L, calculator_subtract);
    lua_settable(L, -3);
    
    lua_pushstring(L, "getValue");
    lua_pushcfunction(L, calculator_getValue);
    lua_settable(L, -3);
    
    lua_pushstring(L, "reset");
    lua_pushcfunction(L, calculator_reset);
    lua_settable(L, -3);
    
    // Pop the metatable
    lua_pop(L, 1);
    
    // Register constructor in global namespace
    lua_pushcfunction(L, calculator_new);
    lua_setglobal(L, "Calculator");
    
    // Register static methods
    lua_newtable(L);
    lua_pushstring(L, "multiply");
    lua_pushcfunction(L, calculator_multiply);
    lua_settable(L, -3);
    lua_setglobal(L, "CalculatorUtils");
}

int main() {
    std::cout << "=== Testing Low-Level Lua Bindings ===" << std::endl;
    
    // Create Lua state
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    
    // Register our bindings
    register_calculator(L);
    std::cout << "Calculator bindings registered successfully!" << std::endl;
    
    // Test 1: Basic usage
    std::cout << "\n--- Test 1: Basic Calculator Usage ---" << std::endl;
    const char* test1 = R"(
        local calc = Calculator()
        print("Initial value:", calc:getValue())
        
        calc:add(10)
        print("After adding 10:", calc:getValue())
        
        calc:subtract(3)
        print("After subtracting 3:", calc:getValue())
        
        calc:reset()
        print("After reset:", calc:getValue())
    )";
    
    if (luaL_dostring(L, test1) != LUA_OK) {
        std::cerr << "Error in test1: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    
    // Test 2: Static methods
    std::cout << "\n--- Test 2: Static Methods ---" << std::endl;
    const char* test2 = R"(
        local result = CalculatorUtils.multiply(6, 7)
        print("6 * 7 =", result)
    )";
    
    if (luaL_dostring(L, test2) != LUA_OK) {
        std::cerr << "Error in test2: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    
    // Test 3: Multiple instances
    std::cout << "\n--- Test 3: Multiple Calculator Instances ---" << std::endl;
    const char* test3 = R"(
        local calc1 = Calculator()
        local calc2 = Calculator()
        
        calc1:add(100)
        calc2:add(200)
        
        print("Calculator 1 value:", calc1:getValue())
        print("Calculator 2 value:", calc2:getValue())
        
        -- They should be independent
        calc1:reset()
        print("After calc1 reset:")
        print("Calculator 1 value:", calc1:getValue())
        print("Calculator 2 value:", calc2:getValue())
    )";
    
    if (luaL_dostring(L, test3) != LUA_OK) {
        std::cerr << "Error in test3: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    
    // Test 4: Error handling
    std::cout << "\n--- Test 4: Error Handling ---" << std::endl;
    const char* test4 = R"(
        local success, error = pcall(function()
            local calc = Calculator()
            calc:add("not a number")  -- This should cause an error
        end)
        
        if not success then
            print("Error caught correctly:", error)
        else
            print("Expected error but none occurred")
        end
    )";
    
    if (luaL_dostring(L, test4) != LUA_OK) {
        std::cerr << "Error in test4: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    
    // Test 5: Performance test
    std::cout << "\n--- Test 5: Performance Test ---" << std::endl;
    const char* test5 = R"(
        local start_time = os.clock()
        local calculators = {}
        
        -- Create 100 calculators and perform operations
        for i = 1, 100 do
            calculators[i] = Calculator()
            calculators[i]:add(i)
            calculators[i]:subtract(1)
            calculators[i]:add(5)
        end
        
        local end_time = os.clock()
        print(string.format("Created and operated 100 calculators in %.3f seconds", end_time - start_time))
        
        -- Verify the last one
        print("Last calculator value:", calculators[100]:getValue())
    )";
    
    if (luaL_dostring(L, test5) != LUA_OK) {
        std::cerr << "Error in test5: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1);
    }
    
    std::cout << "\n=== All Tests Completed! ===" << std::endl;
    std::cout << "This demonstrates the basic patterns that lua_binding_generator would produce." << std::endl;
    std::cout << "Sol2 would handle the boilerplate C wrapper functions automatically." << std::endl;
    
    // Cleanup
    lua_close(L);
    return 0;
}