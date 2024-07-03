add_library(pma_lib STATIC
    pma_btree.cpp
    pma_utils.cpp
)

# Include headers
target_include_directories(pma_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Add the main executable
add_executable(pma_btree
    experiment.cpp
)

# Set output directory for the executable within the src subdirectory context
set_target_properties(pma_btree PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Link the library with the executable
target_link_libraries(pma_btree PRIVATE pma_lib)

# Add debug definitions if in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(pma_btree PRIVATE -g)
    target_compile_definitions(pma_btree PRIVATE -DDEBUG)
endif()
