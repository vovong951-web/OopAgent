cmake_minimum_required(VERSION 3.20)
project(OopAgent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

add_executable(OopAgent
    src/main.cpp
    src/client/ollama_client.cpp
    src/tools/tool_registry.cpp
    src/tools/calculator_tool.cpp
    src/tools/file_tool.cpp
    src/tools/exec_tool.cpp
    src/tools/web_search_tool.cpp
    src/tools/memory_tool.cpp
    src/agent/skill_loader.cpp
    src/agent/agent_loop.cpp
    src/agent/loop_detector.cpp
)

target_include_directories(OopAgent PRIVATE src)

target_link_libraries(OopAgent PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)
# ===== Tuần 5: Integration test =====
add_executable(integration_test
    tests/integration_test.cpp
    src/tools/tool_registry.cpp
    src/tools/calculator_tool.cpp
    src/tools/file_tool.cpp
)

target_include_directories(integration_test PRIVATE src)

target_link_libraries(integration_test PRIVATE
    nlohmann_json::nlohmann_json
)