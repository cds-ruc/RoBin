cmake_minimum_required (VERSION 3.14)
project(GRE_PRO)
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(FetchContent)
include(ExternalProject)

find_package(OpenMP REQUIRED)
find_package(JeMalloc REQUIRED)
find_package(TBB REQUIRED)
find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${TBB_INCLUDE_DIRS})
include_directories(${JEMALLOC_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

add_compile_options(${OpenMP_CXX_FLAGS})
add_compile_options(-faligned-new -march=native -g -O3)

# microbench
add_executable(microbench
    ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark/microbench.cpp
)

target_link_libraries(microbench PUBLIC ${OpenMP_CXX_LIBRARIES} ${JEMALLOC_LIBRARIES} ${TBB_LIBRARIES})
