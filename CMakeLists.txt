cmake_minimum_required(VERSION 3.20)
project(cami
    LANGUAGES CXX
    DESCRIPTION "C Abstract Machine Interpreter")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(disable_rtti)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        add_compile_options(-fno-rtti)
    elseif (MSVC)
        add_compile_options(/GR-)
    endif ()
endfunction()

function(open_leak_sanitizer)
    if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak" PARENT_SCOPE)
        set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=leak" PARENT_SCOPE)
    else ()
        message(WARNING "leak sanitizer is not enabled")
    endif ()
endfunction()

function(set_target_info)
    # check machine word length
    if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "only support 64-bit program")
    endif ()
    # set endian info
    if ("${CMAKE_CXX_BYTE_ORDER}" STREQUAL "BIG_ENDIAN")
        add_compile_definitions(CAMI_TARGET_INFO_BIG_ENDIAN)
    elseif ("${CMAKE_CXX_BYTE_ORDER}" STREQUAL "LITTLE_ENDIAN")
        add_compile_definitions(CAMI_TARGET_INFO_LITTLE_ENDIAN)
    else ()
        message(FATAL_ERROR "unsupported byte order: ${CMAKE_CXX_BYTE_ORDER}")
    endif ()
    # set os info
    if (UNIX)
        add_compile_definitions(CAMI_TARGET_INFO_UNIX_LIKE)
    elseif (WIN32)
        add_compile_definitions(CAMI_TARGET_INFO_WINDOWS)
        add_compile_definitions(NOMINMAX _USE_MATH_DEFINES)
    endif ()
endfunction()

if (CMAKE_BUILD_TYPE STREQUAL "")
    message("CMAKE_BUILD_TYPE implicitly set to \"Release\"")
    set(CMAKE_BUILD_TYPE Release)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    disable_rtti()
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
    open_leak_sanitizer()
endif ()
set_target_info()

include_directories(include)

add_subdirectory(src)

install(TARGETS cami
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )

install(FILES include/cami/io_def.h include/cami/std.h DESTINATION include/cami)