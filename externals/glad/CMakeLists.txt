# SPDX-FileCopyrightText: 2015 Yuri Kunde Schlesner <yuriks@yuriks.net>
# SPDX-License-Identifier: GPL-2.0-or-later

add_library(glad STATIC
    src/glad.c
    include/KHR/khrplatform.h
    include/glad/glad.h
)

create_target_directory_groups(glad)
target_include_directories(glad PUBLIC "include/")

if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
    target_link_libraries(glad PRIVATE dl)
endif()
