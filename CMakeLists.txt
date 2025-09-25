cmake_minimum_required(VERSION 3.10)
project(WebServerDemo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Include current directory for header files
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(
    web_demo
    web_demo.cpp
    db_utils.cpp
    log_utils.cpp
)

# Link SQLite3
target_link_libraries(web_demo PRIVATE SQLite::SQLite3)
