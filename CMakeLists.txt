cmake_minimum_required(VERSION 3.10)
project(NetworkTest VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directory for shared headers
include_directories(${CMAKE_SOURCE_DIR}/source/includes_all/include)

# Server executable
add_executable(server
    source/server/main.cpp
)

# Client executable
add_executable(client
    source/client/main.cpp
)

# Platform-specific linking for networking
if(WIN32)
    target_link_libraries(server ws2_32)
    target_link_libraries(client ws2_32)
elseif(UNIX)
    # Linux doesn't need additional libraries for sockets
endif()

# Set output directories
set_target_properties(server client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
