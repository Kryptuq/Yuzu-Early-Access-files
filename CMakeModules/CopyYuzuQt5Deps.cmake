function(copy_yuzu_Qt5_deps target_dir)
    include(WindowsCopyFiles)
    if (MSVC)
        set(DLL_DEST "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/")
        set(Qt5_DLL_DIR "${Qt5_DIR}/../../../bin")
    else()
        set(DLL_DEST "${CMAKE_BINARY_DIR}/bin/")
        set(Qt5_DLL_DIR "${Qt5_DIR}/../../../lib/")
    endif()
    set(Qt5_PLATFORMS_DIR "${Qt5_DIR}/../../../plugins/platforms/")
    set(Qt5_PLATFORMTHEMES_DIR "${Qt5_DIR}/../../../plugins/platformthemes/")
    set(Qt5_PLATFORMINPUTCONTEXTS_DIR "${Qt5_DIR}/../../../plugins/platforminputcontexts/")
    set(Qt5_XCBGLINTEGRATIONS_DIR "${Qt5_DIR}/../../../plugins/xcbglintegrations/")
    set(Qt5_STYLES_DIR "${Qt5_DIR}/../../../plugins/styles/")
    set(Qt5_IMAGEFORMATS_DIR "${Qt5_DIR}/../../../plugins/imageformats/")
    set(Qt5_RESOURCES_DIR "${Qt5_DIR}/../../../resources/")
    set(PLATFORMS ${DLL_DEST}plugins/platforms/)
    set(STYLES ${DLL_DEST}plugins/styles/)
    set(IMAGEFORMATS ${DLL_DEST}plugins/imageformats/)
    if (MSVC)
        windows_copy_files(${target_dir} ${Qt5_DLL_DIR} ${DLL_DEST}
            icudt*.dll
            icuin*.dll
            icuuc*.dll
            Qt5Core$<$<CONFIG:Debug>:d>.*
            Qt5Gui$<$<CONFIG:Debug>:d>.*
            Qt5Widgets$<$<CONFIG:Debug>:d>.*
        )

        if (YUZU_USE_QT_WEB_ENGINE)
            windows_copy_files(${target_dir} ${Qt5_DLL_DIR} ${DLL_DEST}
                Qt5Network$<$<CONFIG:Debug>:d>.*
                Qt5Positioning$<$<CONFIG:Debug>:d>.*
                Qt5PrintSupport$<$<CONFIG:Debug>:d>.*
                Qt5Qml$<$<CONFIG:Debug>:d>.*
                Qt5QmlModels$<$<CONFIG:Debug>:d>.*
                Qt5Quick$<$<CONFIG:Debug>:d>.*
                Qt5QuickWidgets$<$<CONFIG:Debug>:d>.*
                Qt5WebChannel$<$<CONFIG:Debug>:d>.*
                Qt5WebEngine$<$<CONFIG:Debug>:d>.*
                Qt5WebEngineCore$<$<CONFIG:Debug>:d>.*
                Qt5WebEngineWidgets$<$<CONFIG:Debug>:d>.*
                QtWebEngineProcess$<$<CONFIG:Debug>:d>.*
            )

            windows_copy_files(${target_dir} ${Qt5_RESOURCES_DIR} ${DLL_DEST}
                qtwebengine_resources.pak
                qtwebengine_devtools_resources.pak
                qtwebengine_resources_100p.pak
                qtwebengine_resources_200p.pak
                icudtl.dat
            )
        endif ()
        windows_copy_files(yuzu ${Qt5_PLATFORMS_DIR} ${PLATFORMS} qwindows$<$<CONFIG:Debug>:d>.*)
        windows_copy_files(yuzu ${Qt5_STYLES_DIR} ${STYLES} qwindowsvistastyle$<$<CONFIG:Debug>:d>.*)
        windows_copy_files(yuzu ${Qt5_IMAGEFORMATS_DIR} ${IMAGEFORMATS}
            qjpeg$<$<CONFIG:Debug>:d>.*
            qgif$<$<CONFIG:Debug>:d>.*
            )
    else()
        set(Qt5_DLLS
            "${Qt5_DLL_DIR}libQt5Core.so.5"
            "${Qt5_DLL_DIR}libQt5DBus.so.5"
            "${Qt5_DLL_DIR}libQt5Gui.so.5"
            "${Qt5_DLL_DIR}libQt5Widgets.so.5"
            "${Qt5_DLL_DIR}libQt5XcbQpa.so.5"
            "${Qt5_DLL_DIR}libicudata.so.60"
            "${Qt5_DLL_DIR}libicui18n.so.60"
            "${Qt5_DLL_DIR}libicuuc.so.60"
            )
        set(Qt5_IMAGEFORMAT_DLLS
            "${Qt5_IMAGEFORMATS_DIR}libqjpeg.so"
            "${Qt5_IMAGEFORMATS_DIR}libqgif.so"
            "${Qt5_IMAGEFORMATS_DIR}libqico.so"
            )
        set(Qt5_PLATFORMTHEME_DLLS
            "${Qt5_PLATFORMTHEMES_DIR}libqgtk3.so"
            "${Qt5_PLATFORMTHEMES_DIR}libqxdgdesktopportal.so"
            )
        set(Qt5_PLATFORM_DLLS
            "${Qt5_PLATFORMS_DIR}libqxcb.so"
            )
        set(Qt5_PLATFORMINPUTCONTEXT_DLLS
            "${Qt5_PLATFORMINPUTCONTEXTS_DIR}libcomposeplatforminputcontextplugin.so"
            "${Qt5_PLATFORMINPUTCONTEXTS_DIR}libibusplatforminputcontextplugin.so"
            )
        set(Qt5_XCBGLINTEGRATION_DLLS
            "${Qt5_XCBGLINTEGRATIONS_DIR}libqxcb-glx-integration.so"
            )
        foreach(LIB ${Qt5_DLLS})
            file(COPY ${LIB} DESTINATION "${DLL_DEST}/lib" FOLLOW_SYMLINK_CHAIN)
        endforeach()
        foreach(LIB ${Qt5_IMAGEFORMAT_DLLS})
            file(COPY ${LIB} DESTINATION "${DLL_DEST}plugins/imageformats/" FOLLOW_SYMLINK_CHAIN)
        endforeach()
        foreach(LIB ${Qt5_PLATFORMTHEME_DLLS})
            file(COPY ${LIB} DESTINATION "${DLL_DEST}plugins/platformthemes/" FOLLOW_SYMLINK_CHAIN)
        endforeach()
        foreach(LIB ${Qt5_PLATFORM_DLLS})
            file(COPY ${LIB} DESTINATION "${DLL_DEST}plugins/platforms/" FOLLOW_SYMLINK_CHAIN)
        endforeach()
        foreach(LIB ${Qt5_PLATFORMINPUTCONTEXT_DLLS})
            file(COPY ${LIB} DESTINATION "${DLL_DEST}plugins/platforminputcontexts/" FOLLOW_SYMLINK_CHAIN)
        endforeach()
        foreach(LIB ${Qt5_XCBGLINTEGRATION_DLLS})
            file(COPY ${LIB} DESTINATION "${DLL_DEST}plugins/xcbglintegrations/" FOLLOW_SYMLINK_CHAIN)
        endforeach()

    endif()
    # Create an empty qt.conf file. Qt will detect that this file exists, and use the folder that its in as the root folder.
    # This way it'll look for plugins in the root/plugins/ folder
    add_custom_command(TARGET yuzu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E touch ${DLL_DEST}qt.conf
    )
endfunction(copy_yuzu_Qt5_deps)
