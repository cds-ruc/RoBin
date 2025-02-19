cmake_minimum_required(VERSION 3.14)

# 设置编译器
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

project(RoBin)

# 确保GCC版本不低于11.4.0
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11.4.0)
        message(FATAL_ERROR "GCC version must be at least 11.4.0")
    endif()
endif()

# 添加自定义模块路径
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# 使用FetchContent和ExternalProject管理外部依赖
include(FetchContent)
include(ExternalProject)

# 查找并配置依赖
find_package(OpenMP REQUIRED)
find_package(JeMalloc REQUIRED)
find_package(TBB REQUIRED)
find_package(MKL REQUIRED)
find_package(Boost REQUIRED)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件目录
include_directories(${TBB_INCLUDE_DIRS})
include_directories(${MKL_INCLUDE_DIRS})
include_directories(${JEMALLOC_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

add_compile_options(${OpenMP_CXX_FLAGS})

if(PROFILING)
    message("Compiling for profiling")
    add_definitions(-DPROFILING)
endif()

if(ROOT_PROFILING)
    message("Compiling for root profiling")
    add_definitions(-DROOT_PROFILING)
endif()

# 定义microbench可执行文件
add_executable(microbench ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark/microbench.cpp)

# 针对不同构建类型设置编译选项
target_compile_options(microbench PRIVATE
    $<$<CONFIG:DEBUG>:-g -Og -march=native -faligned-new>
    $<$<CONFIG:RELEASE>:-O3 -march=native -faligned-new>
    $<$<CONFIG:OPTI_ENHANCE>:-O3 -march=native -faligned-new -flto -funroll-all-loops>
)

add_subdirectory(src/competitor/masstree)

target_link_libraries(microbench PRIVATE
    MASSTREE
)

# 链接依赖库
target_link_libraries(microbench PUBLIC 
    ${OpenMP_CXX_LIBRARIES}
    ${JEMALLOC_LIBRARIES}
    ${TBB_LIBRARIES}
    ${MKL_LIBRARIES}
)