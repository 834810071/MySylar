cmake_minimum_required(VERSION 3.13)
project(MySylar)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

set(LIB_SRC
        log.cpp
        log.h
        util.cpp
        util.h
        singleton.h
        config.cpp
        config.h)

add_library(sylar ${LIB_SRC})


add_executable(MySylar main.cpp)
target_link_libraries(MySylar sylar yaml-cpp)

add_subdirectory(test)