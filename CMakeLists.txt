cmake_minimum_required(VERSION 4.1.2)

project(vgsc LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/backend")

file(GLOB SOURCES
    "${CMAKE_SOURCE_DIR}/backend/*.c"
)

find_package(PkgConfig REQUIRED)

find_library(MATH_LIBRARY m)

pkg_check_modules(LIBSYSTEMD REQUIRED IMPORTED_TARGET libsystemd)

add_executable(
    ${PROJECT_NAME}
    ${SOURCES}
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE ${INCLUDE_DIR}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE ${MATH_LIBRARY}
    PRIVATE PkgConfig::LIBSYSTEMD
)
