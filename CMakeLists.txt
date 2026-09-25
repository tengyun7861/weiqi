cmake_minimum_required(VERSION 3.24)
project(weiqi LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep source files in E:\围棋 while allowing builds through the ASCII junction
# E:\weiqi-cmake.  Qt's generated files and MSVC both receive these switches.
if(MSVC)
    add_compile_options(/utf-8 /FS)
endif()

enable_testing()

add_subdirectory(server-cpp)
add_subdirectory(client-qt)
