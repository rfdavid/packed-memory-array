add_executable(pma_index pma_index.cpp pma_helper.cpp experiment.cpp)
add_executable(pma_key_value pma_key_value.cpp experiment.cpp)

set_target_properties(pma_key_value PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

include_directories(${PROJECT_SOURCE_DIR}/include)
set_target_properties(pma_index PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_library(pma_lib STATIC pma_index.cpp pma_helper.cpp)
target_link_libraries(pma_lib)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DDEBUG)
endif()
