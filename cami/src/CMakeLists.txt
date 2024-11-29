add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/include/config.h"
    COMMAND python ARGS "${CMAKE_SOURCE_DIR}/config/configure.py"
    DEPENDS "${CMAKE_SOURCE_DIR}/config/config.toml"
    COMMENT "Generating config.h"
    VERBATIM
)

add_custom_target(generate_config DEPENDS "${CMAKE_SOURCE_DIR}/include/config.h")

function(cami_executable target_name)
    set(sources ${ARGN})
    add_executable(${target_name} ${sources})
    add_dependencies(${target_name} generate_config)
endfunction()

function(cami_library library_name library_type)
    set(sources ${ARGN})
    add_library(${library_name} ${library_type} ${sources})
    add_dependencies(${library_name} generate_config)
endfunction()

add_executable(supporting_test EXCLUDE_FROM_ALL supporting_test.cpp)

file(GLOB_RECURSE libs "${CMAKE_SOURCE_DIR}/include/lib/*.h")
file(GLOB headers "${CMAKE_SOURCE_DIR}/include/*.h")
add_subdirectory(am)
add_subdirectory(foundation)
add_subdirectory(translate)
cami_executable(cami main.cpp launcher.cpp args.cpp ${libs} ${headers})
target_link_libraries(cami PRIVATE am translator)
