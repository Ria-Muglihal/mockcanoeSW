cmake_minimum_required(VERSION 3.10)
project(MockCANoeSW)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug) # 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -g")


file(GLOB_RECURSE FMI2_SRC "FMI2Interface/*.cpp")
file(GLOB_RECURSE COMMON_SRC "common/*.cpp")
file(GLOB_RECURSE CAPL_includes "CAPL_includes/*.h")

set(ALL_SRC main.cpp MockCanoeTick.cpp MockCaplScript.cpp MockCaplSystem.h ${FMI2_SRC} ${COMMON_SRC} ${CAPL_includes})

include_directories(${CMAKE_SOURCE_DIR}/FMI2Interface)
include_directories(${CMAKE_SOURCE_DIR}/common)
include_directories(${CMAKE_SOURCE_DIR}/CAPL_includes)
include_directories(${CMAKE_SOURCE_DIR}/common/fmi2Include)
include_directories(${CMAKE_SOURCE_DIR}/common/PugiXml_1_12)
include_directories(${CMAKE_SOURCE_DIR}/common/utils)

add_executable(mockCanoeSW ${ALL_SRC})

target_link_libraries(mockCanoeSW -ldl -pthread)
