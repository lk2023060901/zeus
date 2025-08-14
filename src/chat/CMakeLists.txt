# Zeus Chat Module CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

message(STATUS "=== Chat Module Configuration ===")

# Chat module sources (to be implemented)
set(CHAT_SOURCES
    # chat_main.cpp
    # chat_server.cpp
    # chat_service.cpp
    # channel_manager.cpp
    # message_filter.cpp
)

# Chat module headers (to be implemented)
set(CHAT_HEADERS
    # ${CMAKE_SOURCE_DIR}/include/chat/chat_server.h
    # ${CMAKE_SOURCE_DIR}/include/chat/chat_service.h
    # ${CMAKE_SOURCE_DIR}/include/chat/channel_manager.h
    # ${CMAKE_SOURCE_DIR}/include/chat/message_filter.h
)

# TODO: Uncomment when source files are implemented
# Create the chat library
# add_library(chat_module ${CHAT_SOURCES} ${CHAT_HEADERS})

# Set target properties
# set_target_properties(chat_module PROPERTIES
#     CXX_STANDARD 17
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
#     OUTPUT_NAME "chat_module"
# )

# Include directories
# target_include_directories(chat_module
#     PUBLIC
#         ${CMAKE_SOURCE_DIR}/include
#     PRIVATE
#         ${CMAKE_CURRENT_SOURCE_DIR}
# )

# Link dependencies
# target_link_libraries(chat_module
#     PUBLIC
#         common_network  # Zeus networking
#         common_spdlog   # Zeus logging
# )

message(STATUS "Chat module configured (placeholder)")
message(STATUS "=====================================")