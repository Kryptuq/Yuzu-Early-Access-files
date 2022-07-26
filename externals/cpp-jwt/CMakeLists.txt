cmake_minimum_required(VERSION 3.14.0)
project(cpp-jwt VERSION 1.5.0)

option(CPP_JWT_BUILD_EXAMPLES "build examples" ON)
option(CPP_JWT_BUILD_TESTS "build tests" ON)
option(CPP_JWT_USE_VENDORED_NLOHMANN_JSON "use vendored json header" ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_BINARY_DIR})

# only set compiler flags if we are the main project, otherwise let the main
# project decide on the flags
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}"
                                                  MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  endif()

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()

endif()

find_package(OpenSSL REQUIRED)

if(NOT CPP_JWT_USE_VENDORED_NLOHMANN_JSON)
  find_package(nlohmann_json REQUIRED)
endif()

# ##############################################################################
# LIBRARY
# ##############################################################################

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(
  ${PROJECT_NAME}
  INTERFACE $<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(${PROJECT_NAME} INTERFACE OpenSSL::SSL)
if(NOT CPP_JWT_USE_VENDORED_NLOHMANN_JSON)
  target_link_libraries(${PROJECT_NAME} INTERFACE nlohmann_json::nlohmann_json)
else()
  target_compile_definitions(${PROJECT_NAME} INTERFACE CPP_JWT_USE_VENDORED_NLOHMANN_JSON)
endif()
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_14)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# ##############################################################################
# TESTS
# ##############################################################################

if(CPP_JWT_BUILD_TESTS)
  find_package(GTest REQUIRED)
  include_directories(${GTEST_INCLUDE_DIRS})
  enable_testing()
  # Recurse into the "Hello" and "Demo" subdirectories. This does not actually
  # cause another cmake executable to run. The same process will walk through
  # the project's entire directory structure.
  add_subdirectory(tests)
endif()

# ##############################################################################
# EXAMPLES
# ##############################################################################

if(CPP_JWT_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ##############################################################################
# INSTALL
# ##############################################################################

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set(CPP_JWT_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(
  EXPORT ${PROJECT_NAME}Targets
  DESTINATION ${CPP_JWT_CONFIG_INSTALL_DIR}
  NAMESPACE ${PROJECT_NAME}::
  COMPONENT dev)
configure_package_config_file(cmake/Config.cmake.in ${PROJECT_NAME}Config.cmake
                              INSTALL_DESTINATION ${CPP_JWT_CONFIG_INSTALL_DIR} 
                              NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake
                                 COMPATIBILITY SameMajorVersion
                                 ARCH_INDEPENDENT)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${CPP_JWT_CONFIG_INSTALL_DIR}
  COMPONENT dev)

if(NOT CPP_JWT_USE_VENDORED_NLOHMANN_JSON)
  set(CPP_JWT_VENDORED_NLOHMANN_JSON_INSTALL_PATTERN PATTERN "json" EXCLUDE)
endif()
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/jwt/
  DESTINATION include/jwt
  COMPONENT dev
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN "*.ipp"
  PATTERN "test" EXCLUDE
  ${CPP_JWT_VENDORED_NLOHMANN_JSON_INSTALL_PATTERN})
