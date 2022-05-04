cmake_minimum_required(VERSION 3.22)

### START OF CONFIGURING GTEST
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)
### END OF CONFIGURING GTEST

# Now simply link against gtest or gtest_main as needed. Eg

add_executable(test_${PROJECT_NAME} main.cpp zagros_test.cpp)
target_link_libraries(test_${PROJECT_NAME} ${PROJECT_NAME} gtest gtest_main)
add_test(NAME test_PROJECT_NAME COMMAND test_${PROJECT_NAME})