# Enhanced Signal Handling Demo
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Enhanced Signal Handling Demo ===")

# Create the demo executable
add_executable(enhanced_signal_demo
    enhanced_signal_demo.cpp
)

# Set target properties
set_target_properties(enhanced_signal_demo PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    OUTPUT_NAME "enhanced_signal_demo"
)

# Compile options
target_compile_options(enhanced_signal_demo
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

message(STATUS "Enhanced Signal Handling Demo configured")
message(STATUS "========================================")