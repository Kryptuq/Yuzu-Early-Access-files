# SPDX-FileCopyrightText: 2016 Citra Emulator Project
# SPDX-License-Identifier: GPL-2.0-or-later

function(copy_yuzu_SDL_deps target_dir)
    include(WindowsCopyFiles)
    set(DLL_DEST "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/")
    windows_copy_files(${target_dir} ${SDL2_DLL_DIR} ${DLL_DEST} SDL2.dll)
endfunction(copy_yuzu_SDL_deps)
