cmake_minimum_required(VERSION 3.24)
project(weiqi LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The source and build directories use ASCII-only paths. Qt's generated files
# and MSVC both receive these switches.
if(MSVC)
    add_compile_options(/utf-8 /FS)
endif()

enable_testing()

add_subdirectory(server-cpp)
add_subdirectory(client-qt)
