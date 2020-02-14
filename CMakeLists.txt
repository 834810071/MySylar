cmake_minimum_required(VERSION 3.13)
project(MySylar)

set(CMAKE_CXX_STANDARD 14)

set(myLib
        log.cpp
        log.h)

add_library(Mylib ${myLib})


add_executable(MySylar main.cpp)
target_link_libraries(MySylar Mylib)