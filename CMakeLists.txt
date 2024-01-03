cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "build type")

project(cpp-repo-template LANGUAGES CXX)
add_executable(${PROJECT_NAME})

find_package(TBB REQUIRED)
find_package(Threads REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

set(PROJECT_WARNINGS_CXX
    -Wall
    -Wextra
    -Wpedantic
    -Werror)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
        $<$<COMPILE_LANGUAGE:CXX>:${PROJECT_WARNINGS_CXX}>
    >
)
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${PROJECT_BINARY_DIR}"
)
target_sources(${PROJECT_NAME} PRIVATE
    src/main.cpp
)
target_link_libraries(
    ${PROJECT_NAME} PRIVATE
    TBB::tbb
    Threads::Threads
    Microsoft.GSL::GSL
)
