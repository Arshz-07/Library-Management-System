cmake_minimum_required(VERSION 3.20)
project(LibroSphere VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_definitions(
        NOMINMAX 
        WIN32_LEAN_AND_MEAN 
        _CRT_SECURE_NO_WARNINGS 
        ASIO_STANDALONE 
        _WIN32_WINNT=0x0601
    )
    add_compile_options(/W3 /permissive- /utf-8)
else()
    add_compile_definitions(ASIO_STANDALONE)
    add_compile_options(-Wall -Wextra)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/crow
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/asio
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/sqlite3
)

# Source files
file(GLOB_RECURSE SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

add_executable(library_server ${SOURCES})

if(WIN32)
    target_link_libraries(library_server PRIVATE ws2_32 wsock32 crypt32 winsqlite3)
else()
    target_link_libraries(library_server PRIVATE pthread dl sqlite3)
endif()

# Copy public static web directory to build directory
add_custom_command(TARGET library_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/public
    $<TARGET_FILE_DIR:library_server>/public
)
