cmake_minimum_required(VERSION 3.20)
project(TurboPalmTree
    VERSION 1.0.0
    DESCRIPTION "Real-time deterministic control system for console automation"
    LANGUAGES CXX)

# Enforce modern C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Architecture & optimization flags
if(MSVC)
    add_compile_options(/arch:AVX2 /fp:fast)
else()
    add_compile_options(-mavx2 -mfma -ffast-math)
endif()

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(src)
