cmake_minimum_required(VERSION 3.24)
project(weiqi_client VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Quick QuickControls2 Network)
qt_standard_project_setup(REQUIRES 6.6)
qt_policy(SET QTP0004 NEW)

qt_add_executable(weiqi_client
    src/main.cpp
    src/game_controller.cpp
    src/game_controller.hpp
    src/network_client.cpp
    src/network_client.hpp
    src/replay_controller.cpp
    src/replay_controller.hpp
)
target_include_directories(weiqi_client PRIVATE ../server-cpp/include)

qt_add_qml_module(weiqi_client
    URI Weiqi
    VERSION 1.0
    QML_FILES
        qml/Main.qml
)

target_link_libraries(weiqi_client PRIVATE weiqi_rules Qt6::Quick Qt6::QuickControls2 Qt6::Network)

set_target_properties(weiqi_client PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
