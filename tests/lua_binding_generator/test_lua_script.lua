#!/usr/bin/env lua

-- test_lua_script.lua
-- Lua script to test the generated bindings

print("=== Lua Binding Test Script ===")

-- Test 1: Basic Vehicle functionality
print("\n--- Test 1: Vehicle Class ---")
local vehicle = test_bindings.Vehicle()
print("Initial speed:", vehicle:getSpeed())

vehicle:setSpeed(60)
print("Speed after setting to 60:", vehicle:getSpeed())
print("Max speed property:", vehicle.maxSpeed)

vehicle:start()
print("Vehicle is running:", vehicle:isRunning())

vehicle:stop()
print("Vehicle stopped, is running:", vehicle:isRunning())

-- Test static method
print("Vehicle count:", test_bindings.Vehicle.getVehicleCount())

-- Test 2: Car class with inheritance  
print("\n--- Test 2: Car Class ---")
local car = test_bindings.Car("Tesla", 100)
print("Car brand:", car:getBrand())
print("Car speed:", car:getSpeed())

-- Test inherited methods
print("Car max speed (inherited):", car.maxSpeed)
car:start()
print("Car is running:", car:isRunning())
print("Engine running:", car:isEngineRunning())

-- Test Car-specific methods
print("Car sound:", car:honk())

-- Test 3: Enum usage
print("\n--- Test 3: Color Enum ---")
car:setColor(Color.Red)
print("Car color set to Red")

car:setColor(Color.Blue)  
print("Car color set to Blue")

-- Test 4: Math utilities (static methods)
print("\n--- Test 4: Math Utils ---")
print("Add 15 + 25 =", test_bindings.MathUtils.add(15, 25))
print("Multiply 2.5 * 4.0 =", test_bindings.MathUtils.multiply(2.5, 4.0))
print("Distance (0,0) to (3,4) =", test_bindings.MathUtils.calculateDistance(0, 0, 3, 4))

-- Test 5: Global functions
print("\n--- Test 5: Global Functions ---")
test_bindings.printMessage("Hello from Lua script!")

local randomNum = test_bindings.getRandomNumber()
print("Random number:", randomNum)

local newCar = test_bindings.createCar("BMW", 150)
print("New car brand:", newCar:getBrand())
print("New car speed:", newCar:getSpeed())

-- Test 6: Constants
print("\n--- Test 6: Constants ---")
print("MAX_SPEED:", test_bindings.MAX_SPEED)
print("PI:", test_bindings.PI)

-- Test 7: Complex interactions
print("\n--- Test 7: Complex Interactions ---")
local fleet = {}

-- Create a fleet of cars
for i = 1, 3 do
    local brand = "Car" .. i
    local speed = 50 + i * 10
    fleet[i] = test_bindings.createCar(brand, speed)
    print(string.format("Created %s with speed %d", fleet[i]:getBrand(), fleet[i]:getSpeed()))
end

-- Start all cars
print("Starting all cars...")
for i, car in ipairs(fleet) do
    car:start()
    print(string.format("%s started - running: %s, engine: %s", 
        car:getBrand(), 
        tostring(car:isRunning()), 
        tostring(car:isEngineRunning())))
end

-- Test 8: Property access
print("\n--- Test 8: Property Access ---")
local testCar = test_bindings.Car("PropertyTest", 80)
testCar.color = Color.Green
print("Set car color to Green via property")

-- Test 9: Error handling (if supported)
print("\n--- Test 9: Error Scenarios ---")
local success, error = pcall(function()
    -- Try some operation that might fail
    local car = test_bindings.Car("", -50)  -- Empty brand, negative speed
    print("Car with empty brand created successfully")
end)

if not success then
    print("Error caught:", error)
else
    print("No errors in edge cases")
end

-- Test 10: Performance test
print("\n--- Test 10: Performance Test ---")
local start_time = os.clock()
local iterations = 1000

for i = 1, iterations do
    local car = test_bindings.Car("TestCar", i)
    car:setSpeed(i * 2)
    local speed = car:getSpeed()
    car:start()
    car:stop()
end

local end_time = os.clock()
local elapsed = end_time - start_time
print(string.format("Created and operated %d cars in %.3f seconds", iterations, elapsed))
print(string.format("Average time per operation: %.6f seconds", elapsed / iterations))

print("\n=== Lua Test Script Completed ===")