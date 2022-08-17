# SPDX-FileCopyrightText: 2017 Citra Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

add_executable(yuzu-room
    yuzu_room.cpp
    yuzu_room.rc
)

create_target_directory_groups(yuzu-room)

target_link_libraries(yuzu-room PRIVATE common core network)
if (ENABLE_WEB_SERVICE)
    target_compile_definitions(yuzu-room PRIVATE -DENABLE_WEB_SERVICE)
    target_link_libraries(yuzu-room PRIVATE web_service)
endif()

target_link_libraries(yuzu-room PRIVATE mbedtls)
if (MSVC)
    target_link_libraries(yuzu-room PRIVATE getopt)
endif()
target_link_libraries(yuzu-room PRIVATE ${PLATFORM_LIBRARIES} Threads::Threads)

if(UNIX AND NOT APPLE)
    install(TARGETS yuzu-room RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
endif()
