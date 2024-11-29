file(GLOB_RECURSE header "${CMAKE_SOURCE_DIR}/include/foundation/*.h")
cami_library(foundation STATIC type/def.cpp type/mm.cpp type/helper.cpp logger.cpp ${header})
target_include_directories(foundation PRIVATE "${CMAKE_SOURCE_DIR}/include/foundation")
