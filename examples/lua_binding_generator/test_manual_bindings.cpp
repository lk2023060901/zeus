/**
 * @file test_manual_bindings.cpp
 * @brief Manual Sol2 bindings example based on lua_binding_generator templates
 * 
 * This file demonstrates what the lua_binding_generator would produce
 * for the test_bindings.hpp file, following the template patterns.
 */

#include "test_bindings.hpp"
#include <sol/sol.hpp>
#include <iostream>
#include <memory>

namespace test_bindings {

// Implementation of functions declared in header
int Vehicle::vehicle_count_ = 0;

std::shared_ptr<Car> createCar(const std::string& brand, int speed) {
    return std::make_shared<Car>(brand, speed);
}

void printMessage(const std::string& message) {
    std::cout << "Message from C++: " << message << std::endl;
}

int getRandomNumber() {
    return 42; // Simplified for testing
}

} // namespace test_bindings

/**
 * @brief Register all test bindings with Sol2
 * This simulates what lua_binding_generator would produce
 */
void register_TestModule_bindings(sol::state& lua) {
    using namespace test_bindings;
    
    // Create namespace table
    sol::table test_bindings_ns = lua.create_named_table("test_bindings");
    
    // Enum bindings - Color enum
    lua.new_enum<Color>("Color", {
        {"Red", Color::Red},
        {"Green", Color::Green},
        {"Blue", Color::Blue}
    });
    test_bindings_ns.set_function("Color", lua["Color"]);
    
    // Vehicle class binding
    lua.new_usertype<Vehicle>("Vehicle",
        // Constructors
        sol::constructors<Vehicle(), Vehicle(int)>(),
        
        // Methods  
        "getSpeed", &Vehicle::getSpeed,
        "setSpeed", &Vehicle::setSpeed,
        "start", &Vehicle::start,
        "stop", &Vehicle::stop,
        "isRunning", &Vehicle::isRunning,
        
        // Properties
        "maxSpeed", sol::property(&Vehicle::getMaxSpeed),
        
        // Static methods
        "getVehicleCount", &Vehicle::getVehicleCount
    );
    test_bindings_ns.set_function("Vehicle", lua["Vehicle"]);
    
    // Car class binding with inheritance
    lua.new_usertype<Car>("Car",
        // Inheritance
        sol::bases<Vehicle>(),
        
        // Constructors
        sol::constructors<Car(), Car(const std::string&, int)>(),
        
        // Methods
        "getBrand", &Car::getBrand,
        "setBrand", &Car::setBrand,
        "getColor", &Car::getColor,
        "setColor", &Car::setColor,
        "start", &Car::start,
        "stop", &Car::stop,
        "isEngineRunning", &Car::isEngineRunning,
        "honk", &Car::honk,
        
        // Properties
        "engineRunning", sol::property(&Car::isEngineRunning),
        "color", sol::property(&Car::getColor, &Car::setColor)
    );
    test_bindings_ns.set_function("Car", lua["Car"]);
    
    // MathUtils static class binding
    lua.new_usertype<MathUtils>("MathUtils",
        "add", &MathUtils::add,
        "multiply", &MathUtils::multiply,
        "calculateDistance", &MathUtils::calculateDistance
    );
    test_bindings_ns.set_function("MathUtils", lua["MathUtils"]);
    
    // Global function bindings
    test_bindings_ns.set_function("createCar", &createCar);
    test_bindings_ns.set_function("printMessage", &printMessage);
    test_bindings_ns.set_function("getRandomNumber", &getRandomNumber);
    
    // Constants
    test_bindings_ns["MAX_SPEED"] = MAX_SPEED;
    test_bindings_ns["PI"] = PI;
}

/**
 * @brief Test function to verify the bindings work correctly
 */
int main() {
    std::cout << "=== Testing Manual Sol2 Bindings ===" << std::endl;
    
    try {
        // Create Lua state
        sol::state lua;
        lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math, sol::lib::io);
        
        // Register our bindings
        register_TestModule_bindings(lua);
        
        std::cout << "Bindings registered successfully!" << std::endl;
        
        // Test 1: Basic class creation and method calls
        std::cout << "\n--- Test 1: Basic Vehicle Class ---" << std::endl;
        lua.script(R"(
            local vehicle = test_bindings.Vehicle()
            print("Initial speed:", vehicle:getSpeed())
            vehicle:setSpeed(50)
            print("Speed after setting to 50:", vehicle:getSpeed())
            print("Max speed:", vehicle.maxSpeed)
            vehicle:start()
            print("Is running:", vehicle:isRunning())
        )");
        
        // Test 2: Inheritance and polymorphism
        std::cout << "\n--- Test 2: Car Class with Inheritance ---" << std::endl;
        lua.script(R"(
            local car = test_bindings.Car("Toyota", 80)
            print("Car brand:", car:getBrand())
            print("Car speed:", car:getSpeed())
            
            -- Test inheritance - Car should have Vehicle methods
            car:start()
            print("Car is running:", car:isRunning())
            print("Engine running:", car:isEngineRunning())
            
            -- Test Car-specific methods
            print("Car honk:", car:honk())
        )");
        
        // Test 3: Enum usage
        std::cout << "\n--- Test 3: Enum Usage ---" << std::endl;
        lua.script(R"(
            local car = test_bindings.Car()
            car:setColor(Color.Red)
            print("Car color set to Red")
            
            -- Try different colors
            car:setColor(Color.Blue)
            print("Car color changed to Blue")
        )");
        
        // Test 4: Static methods
        std::cout << "\n--- Test 4: Static Methods ---" << std::endl;
        lua.script(R"(
            local result = test_bindings.MathUtils.add(10, 20)
            print("MathUtils.add(10, 20) =", result)
            
            local product = test_bindings.MathUtils.multiply(3.14, 2.0)
            print("MathUtils.multiply(3.14, 2.0) =", product)
            
            local distance = test_bindings.MathUtils.calculateDistance(0, 0, 3, 4)
            print("Distance from (0,0) to (3,4) =", distance)
        )");
        
        // Test 5: Global functions
        std::cout << "\n--- Test 5: Global Functions ---" << std::endl;
        lua.script(R"(
            test_bindings.printMessage("Hello from Lua!")
            
            local randomNum = test_bindings.getRandomNumber()
            print("Random number:", randomNum)
            
            local newCar = test_bindings.createCar("Honda", 120)
            print("Created car brand:", newCar:getBrand())
            print("Created car speed:", newCar:getSpeed())
        )");
        
        // Test 6: Constants
        std::cout << "\n--- Test 6: Constants ---" << std::endl;
        lua.script(R"(
            print("MAX_SPEED constant:", test_bindings.MAX_SPEED)
            print("PI constant:", test_bindings.PI)
        )");
        
        // Test 7: Error handling
        std::cout << "\n--- Test 7: Error Handling ---" << std::endl;
        auto result = lua.safe_script(R"(
            local vehicle = test_bindings.Vehicle()
            vehicle:setSpeed(-100)  -- This should work (no validation in our simple example)
            print("Speed set to -100:", vehicle:getSpeed())
        )", sol::script_pass_on_error);
        
        if (!result.valid()) {
            sol::error err = result;
            std::cout << "Error caught: " << err.what() << std::endl;
        }
        
        std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}