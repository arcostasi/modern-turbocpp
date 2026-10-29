cmake_minimum_required(VERSION 3.20)
project(modern_turbo_cpp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Link dynamically against MSVCRT to match prebuilt tvision.lib
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

set(TVISION_ROOT "D:/Code/c/lib/tvision")
set(TVISION_INCLUDE "${TVISION_ROOT}/include")
set(TVISION_LIB "${TVISION_ROOT}/build/Release/tvision.lib")

if(NOT EXISTS "${TVISION_LIB}")
    message(FATAL_ERROR "Could not find tvision.lib at ${TVISION_LIB}. Please verify the path.")
endif()

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${TVISION_INCLUDE}"
)

set(TURBOC_SOURCES
    source/main.cpp
    source/app.cpp
    source/editor.cpp
    source/compiler.cpp
    source/msgwindow.cpp
    source/dialogs.cpp
    source/syntax.cpp
    source/config.cpp
    source/process.cpp
)

add_executable(turboc ${TURBOC_SOURCES})

target_compile_definitions(turboc PRIVATE
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(turboc PRIVATE
        -Wno-deprecated-declarations
        -Wno-macro-redefined
    )
endif()

target_link_libraries(turboc PRIVATE
    "${TVISION_LIB}"
    user32
)

set_target_properties(turboc PROPERTIES
    OUTPUT_NAME "turboc"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# Test engine executable
add_executable(test_engine
    tests/test_engine.cpp
    source/compiler.cpp
    source/syntax.cpp
    source/config.cpp
    source/process.cpp
    source/editor.cpp
)

target_compile_definitions(test_engine PRIVATE
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
)

target_link_libraries(test_engine PRIVATE
    "${TVISION_LIB}"
    user32
)

set_target_properties(test_engine PROPERTIES
    OUTPUT_NAME "test_engine"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)
