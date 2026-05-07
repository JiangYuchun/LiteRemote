cmake_minimum_required(VERSION 3.24)

project(LiteRemote
    VERSION 0.1.0
    DESCRIPTION "Lightweight, low-latency remote assistance software"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LITEREMOTE_BUILD_HOST "Build the LiteRemote host Win32 application" ON)
option(LITEREMOTE_BUILD_CLIENT "Build the LiteRemote client Win32 application" ON)

add_library(literemote_common STATIC
    src/common/protocol.cpp)

target_include_directories(literemote_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_compile_features(literemote_common PUBLIC cxx_std_20)

if(WIN32)
    target_compile_definitions(literemote_common PUBLIC
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE)
else()
    message(STATUS "LiteRemote GUI targets are Windows-only; common library is still configured.")
endif()

if(WIN32 AND LITEREMOTE_BUILD_HOST)
    add_executable(literemote_host WIN32
        apps/host/main.cpp)
    target_link_libraries(literemote_host PRIVATE literemote_common d3d11 dxgi user32 gdi32)
endif()

if(WIN32 AND LITEREMOTE_BUILD_CLIENT)
    add_executable(literemote_client WIN32
        apps/client/main.cpp)
    target_link_libraries(literemote_client PRIVATE literemote_common user32 gdi32 msimg32)
endif()
