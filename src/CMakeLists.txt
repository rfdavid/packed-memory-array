add_executable(pma_key_value pma_key_value.cpp pma_helper.cpp experiment.cpp)

include_directories(${PROJECT_SOURCE_DIR}/include)
set_target_properties(pma_key_value PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_library(pma_key_value_lib STATIC pma_key_value.cpp pma_helper.cpp)
target_link_libraries(pma_key_value_lib)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG)
endif()
