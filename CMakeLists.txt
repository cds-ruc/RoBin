cmake_minimum_required(VERSION 3.14)
project(GRE_PRO)

# 设置编译器
set(CMAKE_C_COMPILER "/usr/bin/gcc")
set(CMAKE_CXX_COMPILER "/usr/bin/g++")

# 添加自定义模块路径
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# 使用FetchContent和ExternalProject管理外部依赖
include(FetchContent)
include(ExternalProject)

# 查找并配置依赖
find_package(OpenMP REQUIRED)
find_package(JeMalloc REQUIRED)
find_package(TBB REQUIRED)
find_package(Boost REQUIRED)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含头文件目录
include_directories(${TBB_INCLUDE_DIRS})
include_directories(${JEMALLOC_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

add_compile_options(${OpenMP_CXX_FLAGS})

add_definitions(-DPROFILING)

# 定义microbench可执行文件
add_executable(microbench ${CMAKE_CURRENT_SOURCE_DIR}/src/benchmark/microbench.cpp)

# 针对不同构建类型设置编译选项
target_compile_options(microbench PRIVATE
    $<$<CONFIG:DEBUG>:-g -Og -march=native -faligned-new>
    $<$<CONFIG:RELEASE>:-O3 -march=native -faligned-new>
    $<$<CONFIG:OPTI_ENHANCE>:-O3 -march=native -faligned-new -flto -funroll-all-loops>
)

# 链接依赖库
target_link_libraries(microbench PUBLIC 
    ${OpenMP_CXX_LIBRARIES}
    ${JEMALLOC_LIBRARIES}
    ${TBB_LIBRARIES}
)