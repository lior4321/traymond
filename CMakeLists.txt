cmake_minimum_required(VERSION 3.20)

# Project setup
project(Traymond LANGUAGES CXX)

# Use C++20 Standard (The newest widely supported standard)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source files
set(SOURCES
    src/traymond.cpp
    src/Traymond.rc
)

# Add Executable (Windows App, not Console)
add_executable(Traymond WIN32 ${SOURCES})

# Modern Compiler Warnings (Force high warning levels)
if(MSVC)
    target_compile_options(Traymond PRIVATE /W4 /permissive- /utf-8)
    # /utf-8 ensures source code is treated as UTF-8
    # /permissive- enforces standard C++ conformance
endif()

# Link required Windows libraries
target_link_libraries(Traymond PRIVATE user32 shell32 advapi32 shlwapi)

# Set Unicode Character Set (Crucial for modern Windows dev)
target_compile_definitions(Traymond PRIVATE UNICODE _UNICODE)
