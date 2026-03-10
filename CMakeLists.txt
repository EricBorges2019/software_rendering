cmake_minimum_required(VERSION 3.20)
project(soft_render VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SIMD "Enable SIMD optimizations" ON)
option(ENABLE_THREADS "Enable multithreading" ON)

find_package(Threads REQUIRED)

# SDL2 — optional, enables the real-time viewer
set(SDL2_PATH "/usr/local/Cellar/sdl2/2.32.6" CACHE PATH "SDL2 install prefix")
find_library(SDL2_LIBRARY NAMES SDL2 HINTS "${SDL2_PATH}/lib")
find_path(SDL2_INCLUDE_DIR SDL2/SDL.h HINTS "${SDL2_PATH}/include")
if(SDL2_LIBRARY AND SDL2_INCLUDE_DIR)
    set(SDL2_FOUND TRUE)
    message(STATUS "Found SDL2: ${SDL2_LIBRARY}")
else()
    message(STATUS "SDL2 not found — viewer will not be built")
endif()

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

add_executable(benchmark_shader
    examples/benchmark_shader.cpp
)
target_link_libraries(benchmark_shader PRIVATE soft_render)

if(SDL2_FOUND)
    add_executable(viewer
        examples/viewer/main.cpp
        examples/demo/obj_loader.cpp
    )
    target_include_directories(viewer PRIVATE
        ${SDL2_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/demo
    )
    target_link_libraries(viewer PRIVATE soft_render ${SDL2_LIBRARY})
endif()

if(WIN32)
    target_compile_definitions(soft_render PUBLIC NOMINMAX)
endif()

option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    enable_testing()

    add_executable(test_vec3 tests/test_vec3.cpp)
    target_link_libraries(test_vec3 PRIVATE soft_render)
    add_test(NAME test_vec3 COMMAND test_vec3)

    add_executable(tests tests/test_math.cpp)
    target_link_libraries(tests PRIVATE soft_render)
    add_test(NAME test_math COMMAND tests)
endif()
