cmake_minimum_required(VERSION 3.20)
project(soft_render VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SIMD "Enable SIMD optimizations" ON)
option(ENABLE_THREADS "Enable multithreading" ON)

find_package(Threads REQUIRED)

add_library(soft_render STATIC
    src/math/vec3.cpp
    src/math/vec4.cpp
    src/math/mat4.cpp
    src/core/framebuffer.cpp
    src/core/texture.cpp
    src/pipeline/vertex_processor.cpp
    src/pipeline/rasterizer.cpp
    src/pipeline/fragment_shader.cpp
    src/render/renderer.cpp
)

target_include_directories(soft_render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ENABLE_SIMD)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
        target_compile_definitions(soft_render PUBLIC USE_SSE4_1)
        target_compile_options(soft_render PUBLIC -msse4.1)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|ARM|aarch64")
        target_compile_definitions(soft_render PUBLIC USE_NEON)
    endif()
endif()

if(ENABLE_THREADS)
    target_compile_definitions(soft_render PUBLIC USE_THREADS)
    target_link_libraries(soft_render PUBLIC Threads::Threads)
endif()

add_executable(demo
    examples/demo/main.cpp
    examples/demo/obj_loader.cpp
)
target_link_libraries(demo PRIVATE soft_render)

if(WIN32)
    target_compile_definitions(soft_render PUBLIC NOMINMAX)
endif()