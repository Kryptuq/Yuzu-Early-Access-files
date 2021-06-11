add_executable(sirit_tests
    main.cpp)
target_link_libraries(sirit_tests PRIVATE sirit)
target_include_directories(sirit_tests PRIVATE . ../include)

add_test(sirit_tests sirit_tests)
