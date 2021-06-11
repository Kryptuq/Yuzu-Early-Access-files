if (TARGET Zycore)
    return()
endif ()

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)
include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

project(Zycore VERSION 1.0.0.0 LANGUAGES C CXX)

# =============================================================================================== #
# Overridable options                                                                             #
# =============================================================================================== #

# Global configuration
option(ZYAN_WHOLE_PROGRAM_OPTIMIZATION
    "Enable whole program optimization (all targets)"
    OFF)
option(ZYAN_NO_LIBC
    "Don't use any C standard library functions (for exotic build-envs like kernel drivers)"
    OFF)
option(ZYAN_DEV_MODE
    "Enable developer mode (-Wall, -Werror, ...)"
    OFF)

# Build configuration
option(ZYCORE_BUILD_SHARED_LIB
    "Build shared library"
    OFF)
option(ZYCORE_BUILD_EXAMPLES
    "Build examples"
    OFF)
option(ZYCORE_BUILD_TESTS
    "Build tests"
    OFF)

# =============================================================================================== #
# GoogleTest                                                                                      #
# =============================================================================================== #

# Download and unpack googletest
if (ZYCORE_BUILD_TESTS)
    if (NOT DEFINED ZYCORE_DOWNLOADED_GTEST)
        configure_file("CMakeLists.txt.in" "${CMAKE_BINARY_DIR}/gtest/download/CMakeLists.txt")
        execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            RESULT_VARIABLE result
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/gtest/download")
        if (result)
            message(FATAL_ERROR "CMake step for googletest failed: ${result}")
        endif()
        execute_process(COMMAND ${CMAKE_COMMAND} --build .
            RESULT_VARIABLE result
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/gtest/download")
        if (result)
            message(FATAL_ERROR "Build step for googletest failed: ${result}")
        endif()

        set(ZYCORE_DOWNLOADED_GTEST TRUE CACHE BOOL "")
        mark_as_advanced(ZYCORE_DOWNLOADED_GTEST)
    endif ()

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    add_subdirectory("${CMAKE_BINARY_DIR}/gtest/src" "${CMAKE_BINARY_DIR}/gtest/build"
        EXCLUDE_FROM_ALL)
endif ()

# =============================================================================================== #
# Exported functions                                                                              #
# =============================================================================================== #

function (zyan_set_common_flags target)
    if (NOT MSVC)
        target_compile_options("${target}" PRIVATE "-std=c99")
    endif ()

    if (ZYAN_DEV_MODE)
        # If in developer mode, be pedantic.
        if (MSVC)
            target_compile_options("${target}" PUBLIC "/WX" "/W4")
        else ()
            target_compile_options("${target}" PUBLIC "-Wall" "-pedantic" "-Wextra" "-Werror")
        endif ()
    endif ()
endfunction ()

function (zyan_set_source_group target)
    if (ZYAN_DEV_MODE)
        if (((CMAKE_MAJOR_VERSION GREATER 3) OR (CMAKE_MAJOR_VERSION EQUAL 3)) AND
            ((CMAKE_MINOR_VERSION GREATER 8) OR (CMAKE_MINOR_VERSION EQUAL 8)))
            # Mirror directory structure in project files
            get_property("TARGET_SOURCE_FILES" TARGET "${target}" PROPERTY SOURCES)
            source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
        endif ()
    endif ()
endfunction ()

function (zyan_maybe_enable_wpo target)
    if (ZYAN_WHOLE_PROGRAM_OPTIMIZATION AND MSVC)
        set_target_properties("${target}" PROPERTIES COMPILE_FLAGS "/GL")
        set_target_properties("${target}" PROPERTIES LINK_FLAGS_RELEASE "/LTCG")
    endif ()
endfunction ()

function (zyan_maybe_enable_wpo_for_lib target)
    if (ZYAN_WHOLE_PROGRAM_OPTIMIZATION AND MSVC)
        set_target_properties("${target}" PROPERTIES COMPILE_FLAGS "/GL")
        set_target_properties("${target}" PROPERTIES LINK_FLAGS_RELEASE "/LTCG")
        set_target_properties("${target}" PROPERTIES STATIC_LIBRARY_FLAGS_RELEASE "/LTCG")
    endif ()
endfunction ()

# =============================================================================================== #
# Library configuration                                                                           #
# =============================================================================================== #

if (ZYCORE_BUILD_SHARED_LIB)
    add_library("Zycore" SHARED)
else ()
    add_library("Zycore" STATIC)
endif ()

set_target_properties("Zycore" PROPERTIES LINKER_LANGUAGE C)
target_include_directories("Zycore"
    PUBLIC "include" ${PROJECT_BINARY_DIR}
    PRIVATE "src")
target_compile_definitions("Zycore" PRIVATE "_CRT_SECURE_NO_WARNINGS" "ZYCORE_EXPORTS")
zyan_set_common_flags("Zycore")
zyan_maybe_enable_wpo_for_lib("Zycore")
generate_export_header("Zycore" BASE_NAME "ZYCORE" EXPORT_FILE_NAME "ZycoreExportConfig.h")

if (ZYAN_NO_LIBC)
    target_compile_definitions("Zycore" PUBLIC "ZYAN_NO_LIBC")
    if (UNIX)
        set_target_properties("Zycore" PROPERTIES LINK_FLAGS "-nostdlib -nodefaultlibs")
    endif ()
endif ()

target_sources("Zycore"
    PRIVATE
        # API
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/API/Memory.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/API/Process.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/API/Synchronization.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/API/Terminal.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/API/Thread.h"
        # Common
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Allocator.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/ArgParse.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Bitset.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Comparison.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Defines.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Format.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/LibC.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/List.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Object.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Status.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/String.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Types.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Vector.h"
        "${CMAKE_CURRENT_LIST_DIR}/include/Zycore/Zycore.h"
        # Common
        "src/Allocator.c"
        "src/ArgParse.c"
        "src/Bitset.c"
        "src/Format.c"
        "src/List.c"
        "src/String.c"
        "src/Vector.c"
        "src/Zycore.c")

if (NOT ZYAN_NO_LIBC)
    target_sources("Zycore"
        PRIVATE
            # API
            "src/API/Memory.c"
            "src/API/Process.c"
            "src/API/Synchronization.c"
            "src/API/Terminal.c"
            "src/API/Thread.c")
endif ()

if (ZYCORE_BUILD_SHARED_LIB AND WIN32)
    target_sources("Zycore" PRIVATE "resources/VersionInfo.rc")
endif ()

zyan_set_source_group("Zycore")

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux" AND NOT ZYAN_NO_LIBC)
    target_compile_definitions("Zycore" PRIVATE "_GNU_SOURCE")
    find_package(Threads REQUIRED)
    target_link_libraries("Zycore" Threads::Threads)
endif ()

configure_package_config_file(cmake/zycore-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/zycore-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/zycore-config.cmake"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake"
)

install(TARGETS "Zycore"
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES
    "${PROJECT_BINARY_DIR}/ZycoreExportConfig.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY "include/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# =============================================================================================== #
# Developer mode                                                                                  #
# =============================================================================================== #

if (ZYAN_DEV_MODE)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif ()

# =============================================================================================== #
# Examples                                                                                        #
# =============================================================================================== #

if (ZYCORE_BUILD_EXAMPLES)
    add_executable("String" "examples/String.c")
    zyan_set_common_flags("String" "Zycore")
    target_link_libraries("String" "Zycore")
    set_target_properties("String" PROPERTIES FOLDER "Examples")
    target_compile_definitions("String" PRIVATE "_CRT_SECURE_NO_WARNINGS")
    zyan_maybe_enable_wpo("String")

    add_executable("Vector" "examples/Vector.c")
    zyan_set_common_flags("Vector" "Zycore")
    target_link_libraries("Vector" "Zycore")
    set_target_properties("Vector" PROPERTIES FOLDER "Examples")
    target_compile_definitions("Vector" PRIVATE "_CRT_SECURE_NO_WARNINGS")
    zyan_maybe_enable_wpo("Vector")
endif ()

# =============================================================================================== #
# Tests                                                                                           #
# =============================================================================================== #

function (zyan_add_test test)
    add_executable("Test${test}" "tests/${test}.cpp")

    if (NOT MSVC)
        target_compile_options("Test${test}" PRIVATE "-std=c++17")
    endif ()

    target_link_libraries("Test${test}" "Zycore")
    target_link_libraries("Test${test}" "gtest")
    set_target_properties("Test${test}" PROPERTIES FOLDER "Tests")
    target_compile_definitions("Test${test}" PRIVATE "_CRT_SECURE_NO_WARNINGS")
    zyan_maybe_enable_wpo("Test${test}")
endfunction ()

if (ZYCORE_BUILD_TESTS)
    zyan_add_test("String")
    zyan_add_test("Vector")
    zyan_add_test("ArgParse")
endif ()

# =============================================================================================== #
