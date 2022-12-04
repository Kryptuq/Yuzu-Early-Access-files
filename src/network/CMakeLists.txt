# SPDX-FileCopyrightText: 2022 yuzu Emulator Project
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(network STATIC
    announce_multiplayer_session.cpp
    announce_multiplayer_session.h
    network.cpp
    network.h
    packet.cpp
    packet.h
    precompiled_headers.h
    room.cpp
    room.h
    room_member.cpp
    room_member.h
    verify_user.cpp
    verify_user.h
)

create_target_directory_groups(network)

target_link_libraries(network PRIVATE common enet::enet Boost::boost)
if (ENABLE_WEB_SERVICE)
    target_compile_definitions(network PRIVATE -DENABLE_WEB_SERVICE)
    target_link_libraries(network PRIVATE web_service)
endif()

if (YUZU_USE_PRECOMPILED_HEADERS)
    target_precompile_headers(network PRIVATE precompiled_headers.h)
endif()
