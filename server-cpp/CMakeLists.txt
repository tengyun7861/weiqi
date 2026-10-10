cmake_minimum_required(VERSION 3.24)
project(weiqi_server VERSION 0.2.0 LANGUAGES C CXX)

include(FetchContent)

# Runtime dependencies are pinned to fixed source revisions for reproducible
# CMake configuration and local deployment.
set(CROW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CROW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(asio
    URL https://codeload.github.com/chriskohlhoff/asio/zip/42a93679dc4c8c5caf3d3082542f1bfa2438271d
    URL_HASH SHA256=b7c64b502a99d98f584958ae4af2a7ef9857511a9f80e246735c3a3bff29857e)
FetchContent_Declare(crow
    URL https://codeload.github.com/CrowCpp/Crow/zip/b4c1b4f58a25eeab5fb64287baf8860f1a48e763
    URL_HASH SHA256=af0072cb2b78a74978855296c4990a950b6c8176f4ebf74407cb7d2d2fa93a85)
FetchContent_Declare(nlohmann_json
    URL https://codeload.github.com/nlohmann/json/zip/9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03
    URL_HASH SHA256=42013e884a8a38d2a974619844a294de614dab4bc14e0ad23711ef4dacbb24c1)
FetchContent_Declare(sqlite3
    URL https://www.sqlite.org/2025/sqlite-amalgamation-3500400.zip
    URL_HASH SHA3_256=f131b68e6ba5fb891cc13ebb5ff9555054c77294cb92d8d1268bad5dba4fa2a1)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "Standalone Asio headers" FORCE)
FetchContent_MakeAvailable(crow nlohmann_json sqlite3)

file(GLOB_RECURSE SQLITE3_AMALGAMATION CONFIGURE_DEPENDS "${sqlite3_SOURCE_DIR}/sqlite3.c")
if(NOT SQLITE3_AMALGAMATION)
    message(FATAL_ERROR "未在 SQLite amalgamation 下载包中找到 sqlite3.c")
endif()
get_filename_component(SQLITE3_INCLUDE_DIR "${SQLITE3_AMALGAMATION}" DIRECTORY)
add_library(weiqi_sqlite STATIC "${SQLITE3_AMALGAMATION}")
target_include_directories(weiqi_sqlite PUBLIC "${SQLITE3_INCLUDE_DIR}")
target_compile_definitions(weiqi_sqlite PRIVATE SQLITE_THREADSAFE=1 SQLITE_OMIT_LOAD_EXTENSION=1)

add_library(weiqi_rules src/go_board.cpp src/sgf.cpp src/replay.cpp)
target_include_directories(weiqi_rules PUBLIC include)

add_library(weiqi_server_core
    src/auth.cpp
    src/database.cpp
    src/services/rooms_service.cpp
    src/services/game_state_service.cpp
    src/services/persistence_service.cpp
    src/services/clock_service.cpp
    src/services/gameplay_service.cpp
    src/services/ai_service.cpp
    src/services/matchmaking_service.cpp
    src/services/chat_service.cpp
    src/services/records_service.cpp
    src/katago_gtp.cpp
    src/ai_worker.cpp
)
target_include_directories(weiqi_server_core PUBLIC include)
target_link_libraries(weiqi_server_core PUBLIC weiqi_rules weiqi_sqlite nlohmann_json::nlohmann_json)
if(WIN32)
    target_link_libraries(weiqi_server_core PRIVATE bcrypt)
endif()

add_executable(weiqi_server
    src/main.cpp src/api_server.cpp src/web_assets.cpp
    src/api/http_common.cpp src/api/serialization.cpp
    src/api/system_routes.cpp src/api/asset_routes.cpp src/api/auth_routes.cpp
    src/api/room_routes.cpp src/api/record_routes.cpp src/api/replay_routes.cpp src/api/ai_routes.cpp
    src/api/realtime_server.cpp src/api/realtime_rooms.cpp
    src/api/realtime_matchmaking.cpp src/api/realtime_gameplay.cpp src/api/realtime_chat.cpp)
target_link_libraries(weiqi_server PRIVATE weiqi_server_core Crow::Crow)
target_compile_definitions(weiqi_server PRIVATE
    CROW_DISABLE_STATIC_DIR
    WEIQI_DEFAULT_WEB_ROOT="${CMAKE_SOURCE_DIR}/client-web/dist")
install(TARGETS weiqi_server RUNTIME DESTINATION bin)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/client-web/dist/ DESTINATION bin/web OPTIONAL)
if(WIN32)
    target_compile_definitions(weiqi_server PRIVATE _WIN32_WINNT=0x0A00 WINVER=0x0A00)
endif()

if(BUILD_TESTING)
add_executable(weiqi_rules_test tests/go_board_test.cpp)
target_link_libraries(weiqi_rules_test PRIVATE weiqi_rules)

add_executable(weiqi_server_core_test tests/server_core_test.cpp)
target_link_libraries(weiqi_server_core_test PRIVATE weiqi_server_core)

add_executable(weiqi_gtp_stub tests/gtp_stub.cpp)
add_executable(weiqi_katago_gtp_test tests/katago_gtp_test.cpp)
target_link_libraries(weiqi_katago_gtp_test PRIVATE weiqi_server_core)
add_executable(weiqi_api_smoke_test tests/api_smoke_test.cpp)
add_executable(weiqi_websocket_e2e_test tests/websocket_e2e_test.cpp)
target_link_libraries(weiqi_websocket_e2e_test PRIVATE weiqi_server_core)
if(WIN32)
    target_link_libraries(weiqi_api_smoke_test PRIVATE ws2_32)
    target_link_libraries(weiqi_websocket_e2e_test PRIVATE ws2_32)
endif()

enable_testing()
add_test(NAME weiqi_rules_test COMMAND weiqi_rules_test)
add_test(NAME weiqi_server_core_test COMMAND weiqi_server_core_test)
add_test(NAME weiqi_katago_gtp_test COMMAND weiqi_katago_gtp_test $<TARGET_FILE:weiqi_gtp_stub>)
add_test(NAME weiqi_api_smoke_test COMMAND weiqi_api_smoke_test $<TARGET_FILE:weiqi_server>)
add_test(NAME weiqi_websocket_e2e_test COMMAND weiqi_websocket_e2e_test $<TARGET_FILE:weiqi_server>)
set_tests_properties(weiqi_websocket_e2e_test PROPERTIES TIMEOUT 25)
add_executable(weiqi_web_assets_test tests/web_assets_test.cpp src/web_assets.cpp)
target_include_directories(weiqi_web_assets_test PRIVATE include)
add_test(NAME weiqi_web_assets_test COMMAND weiqi_web_assets_test)
add_executable(weiqi_replay_test tests/replay_test.cpp)
target_link_libraries(weiqi_replay_test PRIVATE weiqi_rules)
add_test(NAME weiqi_replay_test COMMAND weiqi_replay_test)
set_tests_properties(weiqi_replay_test PROPERTIES TIMEOUT 10)
add_executable(weiqi_ai_worker_test tests/ai_worker_test.cpp)
target_link_libraries(weiqi_ai_worker_test PRIVATE weiqi_server_core)
add_test(NAME weiqi_ai_worker_test COMMAND weiqi_ai_worker_test $<TARGET_FILE:weiqi_gtp_stub>)
set_tests_properties(weiqi_ai_worker_test PROPERTIES TIMEOUT 15)
endif()
