cmake_minimum_required(VERSION 3.20)
project(OopAgent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(OopAgent
    src/main.cpp
    src/client/ollama_client.cpp
    src/tools/tool_registry.cpp
    src/tools/calculator_tool.cpp
    src/tools/file_tool.cpp
    src/tools/exec_tool.cpp
)

target_include_directories(OopAgent PRIVATE src)

target_link_libraries(OopAgent PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
)