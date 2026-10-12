cmake_minimum_required(VERSION 3.16)

project(unix-shell C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(shll
    src/main.c
    src/get_path.c
)

target_include_directories(shll PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(shll PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)