if ((NOT "A32" IN_LIST DYNARMIC_FRONTENDS) OR (NOT "A64" IN_LIST DYNARMIC_FRONTENDS))
    return()
endif()

add_executable(dynarmic_tests
    A32/test_arm_disassembler.cpp
    A32/test_arm_instructions.cpp
    A32/test_thumb_instructions.cpp
    A32/testenv.h
    A64/a64.cpp
    A64/testenv.h
    cpu_info.cpp
    decoder_tests.cpp
    fp/FPToFixed.cpp
    fp/FPValue.cpp
    fp/mantissa_util_tests.cpp
    fp/unpacked_tests.cpp
    main.cpp
    rand_int.h
)

if (NOT MSVC)
    target_sources(dynarmic_tests PRIVATE
        rsqrt_test.cpp
        rsqrt_test_fn.s
    )
endif()

if (DYNARMIC_TESTS_USE_UNICORN)
    target_sources(dynarmic_tests PRIVATE
        A32/fuzz_arm.cpp
        A32/fuzz_thumb.cpp
        A64/fuzz_with_unicorn.cpp
        A64/misaligned_page_table.cpp
        A64/verify_unicorn.cpp
        fuzz_util.cpp
        fuzz_util.h
        unicorn_emu/a32_unicorn.cpp
        unicorn_emu/a32_unicorn.h
        unicorn_emu/a64_unicorn.cpp
        unicorn_emu/a64_unicorn.h
    )
    target_link_libraries(dynarmic_tests PRIVATE Unicorn::Unicorn)
endif()

add_executable(dynarmic_print_info
    print_info.cpp
)

include(CreateDirectoryGroups)
create_target_directory_groups(dynarmic_tests)
create_target_directory_groups(dynarmic_print_info)

target_link_libraries(dynarmic_tests PRIVATE dynarmic boost catch fmt xbyak)
target_include_directories(dynarmic_tests PRIVATE . ../src)
target_compile_options(dynarmic_tests PRIVATE ${DYNARMIC_CXX_FLAGS})
target_compile_definitions(dynarmic_tests PRIVATE FMT_USE_USER_DEFINED_LITERALS=1 CATCH_CONFIG_ENABLE_BENCHMARKING=1)

target_link_libraries(dynarmic_print_info PRIVATE dynarmic boost catch fmt)
target_include_directories(dynarmic_print_info PRIVATE . ../src)
target_compile_options(dynarmic_print_info PRIVATE ${DYNARMIC_CXX_FLAGS})
target_compile_definitions(dynarmic_print_info PRIVATE FMT_USE_USER_DEFINED_LITERALS=1)

add_test(dynarmic_tests dynarmic_tests --durations yes)
