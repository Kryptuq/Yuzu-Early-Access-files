add_library(audio_core STATIC
    algorithm/filter.cpp
    algorithm/filter.h
    algorithm/interpolate.cpp
    algorithm/interpolate.h
    audio_out.cpp
    audio_out.h
    audio_renderer.cpp
    audio_renderer.h
    behavior_info.cpp
    behavior_info.h
    buffer.h
    codec.cpp
    codec.h
    command_generator.cpp
    command_generator.h
    common.h
    delay_line.cpp
    delay_line.h
    effect_context.cpp
    effect_context.h
    info_updater.cpp
    info_updater.h
    memory_pool.cpp
    memory_pool.h
    mix_context.cpp
    mix_context.h
    null_sink.h
    sink.h
    sink_context.cpp
    sink_context.h
    sink_details.cpp
    sink_details.h
    sink_stream.h
    splitter_context.cpp
    splitter_context.h
    stream.cpp
    stream.h
    time_stretch.cpp
    time_stretch.h
    voice_context.cpp
    voice_context.h

    $<$<BOOL:${ENABLE_CUBEB}>:cubeb_sink.cpp cubeb_sink.h>
    $<$<BOOL:${ENABLE_SDL2}>:sdl2_sink.cpp sdl2_sink.h>
)

create_target_directory_groups(audio_core)

if (NOT MSVC)
    target_compile_options(audio_core PRIVATE
        -Werror=conversion
        -Werror=ignored-qualifiers
        -Werror=shadow
        -Werror=unused-parameter
        -Werror=unused-variable

        $<$<CXX_COMPILER_ID:GNU>:-Werror=unused-but-set-parameter>
        $<$<CXX_COMPILER_ID:GNU>:-Werror=unused-but-set-variable>

        -Wno-sign-conversion
    )
endif()

target_link_libraries(audio_core PUBLIC common core)
target_link_libraries(audio_core PRIVATE SoundTouch)

if(ENABLE_CUBEB)
    target_link_libraries(audio_core PRIVATE cubeb)
    target_compile_definitions(audio_core PRIVATE -DHAVE_CUBEB=1)
endif()
if(ENABLE_SDL2)
    target_link_libraries(audio_core PRIVATE SDL2)
    target_compile_definitions(audio_core PRIVATE HAVE_SDL2)
endif()
