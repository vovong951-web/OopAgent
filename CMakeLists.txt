cmake_minimum_required(VERSION 3.20)
project(OopAgent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

include_directories(src)

add_executable(agent
    src/main.cpp
    src/client/ollama_client.cpp
)

target_link_libraries(agent
    CURL::libcurl
    nlohmann_json::nlohmann_json
    SQLite::SQLite3
)