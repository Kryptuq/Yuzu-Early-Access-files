add_executable(mcl-tests
    bit/bit_field_tests.cpp
    main.cpp
    mp/metavalue_tests.cpp
    mp/typelist_tests.cpp
    type_traits/type_traits_tests.cpp
)
target_include_directories(mcl-tests PUBLIC .)
target_compile_options(mcl-tests PRIVATE ${STAMINA_CXX_FLAGS})
target_link_libraries(mcl-tests PRIVATE Catch2::Catch2 mcl)

include(CTest)
include(Catch)
catch_discover_tests(mcl-tests)
enable_testing()
