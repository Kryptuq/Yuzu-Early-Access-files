cmake_minimum_required(VERSION 3.8)
project(oaknut LANGUAGES CXX VERSION 0.0.0)

# Determine if we're built as a subproject (using add_subdirectory)
# or if this is the master project.
set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif()

# Disable in-source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(SEND_ERROR "In-source builds are not allowed.")
endif()

# Source project files
set(header_files
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/arm64_encode_helpers.inc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/arm64_mnemonics.inc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/enum.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/fpsimd_mnemonics.inc.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/imm.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/list.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/multi_typed_name.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/offset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/reg.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/impl/string_literal.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/oaknut/oaknut.hpp
)

# Library definition
add_library(oaknut INTERFACE)
add_library(merry::oaknut ALIAS oaknut)
target_sources(oaknut INTERFACE "$<BUILD_INTERFACE:${header_files}>")
target_include_directories(oaknut INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(oaknut INTERFACE cxx_std_20)

# Tests
if (MASTER_PROJECT)
    find_package(Catch2 3 REQUIRED)

    add_executable(oaknut-tests
        tests/basic.cpp
        tests/fpsimd.cpp
        tests/general.cpp
    )
    target_include_directories(oaknut-tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    target_link_libraries(oaknut-tests PRIVATE Catch2::Catch2WithMain merry::oaknut)
    target_compile_options(oaknut-tests PRIVATE -Wall -Wextra -Wcast-qual -pedantic -pedantic-errors -Wfatal-errors -Wno-missing-braces)

    include(CTest)
    include(Catch)
    catch_discover_tests(oaknut-tests)
    enable_testing()
endif()
