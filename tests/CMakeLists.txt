find_package(Catch2 3 REQUIRED)
add_executable(tests test_main.cpp)

include_directories(${PROJECT_SOURCE_DIR}/include)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain pma_key_value_lib)

include(Catch)
catch_discover_tests(tests)
