# SPDX-FileCopyrightText: 2018 yuzu Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

# Credits to Samantas5855 and others for this function.
function(create_resource file output filename)
    # Read hex data from file
    file(READ ${file} filedata HEX)
    # Convert hex data for C compatibility
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
    # Write data to output file
    set(RESOURCES_DIR "${PROJECT_BINARY_DIR}/dist" PARENT_SCOPE)
    file(WRITE "${PROJECT_BINARY_DIR}/dist/${output}" "const unsigned char ${filename}[] = {${filedata}};\nconst unsigned ${filename}_size = sizeof(${filename});\n")
endfunction()

add_executable(yuzu-cmd
    config.cpp
    config.h
    default_ini.h
    emu_window/emu_window_sdl2.cpp
    emu_window/emu_window_sdl2.h
    emu_window/emu_window_sdl2_gl.cpp
    emu_window/emu_window_sdl2_gl.h
    emu_window/emu_window_sdl2_null.cpp
    emu_window/emu_window_sdl2_null.h
    emu_window/emu_window_sdl2_vk.cpp
    emu_window/emu_window_sdl2_vk.h
    precompiled_headers.h
    yuzu.cpp
    yuzu.rc
)

create_target_directory_groups(yuzu-cmd)

target_link_libraries(yuzu-cmd PRIVATE common core input_common)
target_link_libraries(yuzu-cmd PRIVATE inih::INIReader glad)
if (MSVC)
    target_link_libraries(yuzu-cmd PRIVATE getopt)
endif()
target_link_libraries(yuzu-cmd PRIVATE ${PLATFORM_LIBRARIES} Threads::Threads)

create_resource("../../dist/yuzu.bmp" "yuzu_cmd/yuzu_icon.h" "yuzu_icon")
target_include_directories(yuzu-cmd PRIVATE ${RESOURCES_DIR})

target_link_libraries(yuzu-cmd PRIVATE Vulkan::Headers)

if (YUZU_USE_EXTERNAL_SDL2)
    target_link_libraries(yuzu-cmd PRIVATE SDL2-static)
else()
    target_link_libraries(yuzu-cmd PRIVATE SDL2)
endif()

if(UNIX AND NOT APPLE)
    install(TARGETS yuzu-cmd)
endif()

if (MSVC)
    include(CopyYuzuSDLDeps)
    copy_yuzu_SDL_deps(yuzu-cmd)
endif()

if (YUZU_USE_PRECOMPILED_HEADERS)
    target_precompile_headers(yuzu-cmd PRIVATE precompiled_headers.h)
endif()
