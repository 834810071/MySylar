cmake_minimum_required(VERSION 3.13)
project(MySylar)

set(CMAKE_CXX_STANDARD 14)

add_executable(test_log test_log.cpp)
target_link_libraries(test_log Mylib)