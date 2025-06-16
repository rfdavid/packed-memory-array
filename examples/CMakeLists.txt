# Create an interface library for the header-only PMA
add_library(pma_lib INTERFACE)

# Include headers
target_include_directories(pma_lib INTERFACE ${PROJECT_SOURCE_DIR}/src)

# Add the main executable
add_executable(pma
    experiment.cpp
)

# Set output directory for the executable
set_target_properties(pma PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Include the header directory
target_include_directories(pma PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link the library with the executable
target_link_libraries(pma PRIVATE pma_lib)

# Add debug definitions if in Debug mode
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(pma PRIVATE -g)
    target_compile_definitions(pma PRIVATE -DDEBUG)
endif()
