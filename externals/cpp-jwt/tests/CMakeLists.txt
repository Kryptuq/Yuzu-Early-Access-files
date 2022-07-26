set(CERT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/certs")
set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -DCERT_ROOT_DIR=\"\\\"${CERT_ROOT_DIR}\\\"\"")

add_executable(test_jwt_object test_jwt_object.cc)
target_link_libraries(test_jwt_object GTest::GTest GTest::Main ${PROJECT_NAME})
target_include_directories(test_jwt_object PRIVATE ${GTEST_INCLUDE_DIRS}
                                                   ${GTest_INCLUDE_DIRS})
add_test(
  NAME test_jwt_object
  COMMAND ./test_jwt_object
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_jwt_encode test_jwt_encode.cc)
target_link_libraries(test_jwt_encode GTest::GTest GTest::Main ${PROJECT_NAME})
target_include_directories(test_jwt_encode PRIVATE ${GTEST_INCLUDE_DIRS}
                                                   ${GTest_INCLUDE_DIRS})
add_test(
  NAME test_jwt_encode
  COMMAND ./test_jwt_encode
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_jwt_decode test_jwt_decode.cc)
target_link_libraries(test_jwt_decode GTest::GTest GTest::Main ${PROJECT_NAME})
target_include_directories(test_jwt_decode PRIVATE ${GTEST_INCLUDE_DIRS}
                                                   ${GTest_INCLUDE_DIRS})
add_test(
  NAME test_jwt_decode
  COMMAND ./test_jwt_decode
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_jwt_decode_verifiy test_jwt_decode_verifiy.cc)
target_link_libraries(test_jwt_decode_verifiy GTest::GTest GTest::Main
                      ${PROJECT_NAME})
target_include_directories(test_jwt_decode_verifiy
                           PRIVATE ${GTEST_INCLUDE_DIRS} ${GTest_INCLUDE_DIRS})
add_test(
  NAME test_jwt_decode_verifiy
  COMMAND ./test_jwt_decode_verifiy
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_jwt_decode_verifiy_with_exception
               test_jwt_decode_verifiy_with_exception.cc)
target_link_libraries(test_jwt_decode_verifiy_with_exception GTest::GTest GTest::Main
                      ${PROJECT_NAME})
target_include_directories(test_jwt_decode_verifiy_with_exception
                           PRIVATE ${GTEST_INCLUDE_DIRS} ${GTest_INCLUDE_DIRS})
add_test(
  NAME test_jwt_decode_verifiy_with_exception
  COMMAND ./test_jwt_decode_verifiy_with_exception
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_jwt_rsa test_jwt_rsa.cc)
target_link_libraries(test_jwt_rsa GTest::GTest GTest::Main ${PROJECT_NAME})
target_include_directories(test_jwt_rsa PRIVATE ${GTEST_INCLUDE_DIRS}
                                                ${GTest_INCLUDE_DIRS})
add_test(
  NAME test_jwt_rsa
  COMMAND ./test_jwt_rsa
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test_jwt_es test_jwt_es.cc)
target_link_libraries(test_jwt_es GTest::GTest GTest::Main ${PROJECT_NAME})
target_include_directories(test_jwt_es PRIVATE ${GTEST_INCLUDE_DIRS}
                                               ${GTest_INCLUDE_DIRS})
add_test(
  NAME test_jwt_es
  COMMAND ./test_jwt_es
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
