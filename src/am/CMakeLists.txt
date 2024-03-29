if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(eval_src evaluation/cast.cc evaluation/raw_value_eval.cc evaluation/valuebox_eval.cc evaluation/am_eval.cc)
else ()
    set(eval_src evaluation.cc)
endif ()

file(GLOB_RECURSE header "${CMAKE_SOURCE_DIR}/include/am/*.h")
cami_library(am STATIC am.cpp fetch_decode.cpp execute.cpp vmm.cpp object.cpp obj_man.cpp
    heap_allocator.cpp trace.cpp ub.cpp formatter.cpp ${eval_src} ${header})
target_include_directories(am PRIVATE "${CMAKE_SOURCE_DIR}/include/am")
target_link_libraries(am PUBLIC foundation)