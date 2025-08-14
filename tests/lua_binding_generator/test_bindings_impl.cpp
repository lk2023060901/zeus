#include "test_bindings.hpp"
#include <iostream>
#include <cmath>

namespace test_bindings {

// Vehicle static member
int Vehicle::vehicle_count_ = 0;

// Global function implementations
std::shared_ptr<Car> createCar(const std::string& brand, int speed) {
    return std::make_shared<Car>(brand, speed);
}

void printMessage(const std::string& message) {
    std::cout << "C++ says: " << message << std::endl;
}

int getRandomNumber() {
    return 42; // Deterministic for testing
}

} // namespace test_bindings