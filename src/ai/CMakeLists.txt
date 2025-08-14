# Zeus AI Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== AI Module Configuration ===")

# AI module sources (to be implemented)
set(AI_SOURCES
    # ai_main.cpp
    # ai_server.cpp
    # npc_manager.cpp
    # behavior_tree.cpp
    # decision_engine.cpp
)

# AI module headers (to be implemented)
set(AI_HEADERS
    # ${CMAKE_SOURCE_DIR}/include/ai/ai_server.h
    # ${CMAKE_SOURCE_DIR}/include/ai/npc_manager.h
    # ${CMAKE_SOURCE_DIR}/include/ai/behavior_tree.h
    # ${CMAKE_SOURCE_DIR}/include/ai/decision_engine.h
)

# TODO: Uncomment when source files are implemented
# Create the AI library
# add_library(ai_module ${AI_SOURCES} ${AI_HEADERS})

# Set target properties
# set_target_properties(ai_module PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     OUTPUT_NAME "ai_module"
# )

# Include directories
# target_include_directories(ai_module
#     PUBLIC
#         ${CMAKE_SOURCE_DIR}/include
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# Link dependencies
# target_link_libraries(ai_module
#     PUBLIC
#         common_network  # Zeus networking
#         common_spdlog   # Zeus logging
# )

message(STATUS "AI module configured (placeholder)")
message(STATUS "==================================")