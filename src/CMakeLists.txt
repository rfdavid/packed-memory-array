add_library(pma_lib STATIC
    pma.cpp
    pma_utils.cpp
)

# Include headers
target_include_directories(pma_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Add the main executable
add_executable(pma
    experiment.cpp
)

# Set output directory for the executable within the src subdirectory context
set_target_properties(pma PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Link the library with the executable
target_link_libraries(pma PRIVATE pma_lib)

# Add debug definitions if in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(pma PRIVATE -g)
    target_compile_definitions(pma PRIVATE -DDEBUG)
endif()
