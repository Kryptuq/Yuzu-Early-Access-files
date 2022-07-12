# Always build externals as static libraries, even when dynarmic is built as shared
if (BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# For libraries that already come with a CMakeLists file,
# simply add the directory to that file as a subdirectory
# to have CMake automatically recognize them.

# catch

if (DYNARMIC_TESTS AND NOT TARGET catch)
    add_library(catch INTERFACE)
    target_include_directories(catch INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/catch/include>)
endif()

# fmt

if (NOT TARGET fmt AND NOT TARGET fmt::fmt)
    # fmtlib formatting library
    add_subdirectory(fmt)
endif()

# mcl

if (NOT TARGET merry::mcl)
    add_subdirectory(mcl)
endif()

# oaknut

if (NOT TARGET merry::oaknut)
    if (ARCHITECTURE STREQUAL "arm64")
        add_subdirectory(oaknut)
    endif()
endif()

# robin-map

if (NOT TARGET tsl::robin_map)
    add_subdirectory(robin-map)
endif()

# xbyak

if (NOT TARGET xbyak)
    if (ARCHITECTURE STREQUAL "x86" OR ARCHITECTURE STREQUAL "x86_64")
        add_library(xbyak INTERFACE)
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/xbyak/include)
        file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/xbyak/xbyak DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/xbyak/include)
        target_include_directories(xbyak SYSTEM INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/xbyak/include)
        target_compile_definitions(xbyak INTERFACE XBYAK_NO_OP_NAMES)
    endif()
endif()

# zydis

if (NOT TARGET Zydis)
    option(ZYDIS_BUILD_TOOLS "" OFF)
    option(ZYDIS_BUILD_EXAMPLES "" OFF)
    set(ZYDIS_ZYCORE_PATH "${CMAKE_CURRENT_LIST_DIR}/zycore" CACHE PATH "")
    add_subdirectory(zydis EXCLUDE_FROM_ALL)
endif()
