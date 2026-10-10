cmake_minimum_required(VERSION 3.24)
project(weiqi VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The application is a C++ HTTP/WebSocket server with a browser frontend.
if(MSVC)
    add_compile_options(/utf-8 /FS)
endif()

include(CTest)

add_subdirectory(server-cpp)
