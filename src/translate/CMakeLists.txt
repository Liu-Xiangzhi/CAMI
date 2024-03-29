file(GLOB_RECURSE header "${CMAKE_SOURCE_DIR}/include/translate/*.h")
set(assembler_source assembler/lexer.cpp assembler/entry.cpp assembler/attribute.cpp
    assembler/code.cpp assembler/data.cpp assembler/entity.cpp)
add_library(translator STATIC ${assembler_source} deassembler.cpp linker.cpp ${header})
target_include_directories(translator PRIVATE "${CMAKE_SOURCE_DIR}/include/translate")
target_link_libraries(translator PUBLIC foundation)
