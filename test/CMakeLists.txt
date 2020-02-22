cmake_minimum_required(VERSION 3.13)
project(MySylar)

set(CMAKE_CXX_STANDARD 14)

add_executable(test_log test_log.cpp)
add_executable(test_config test_config.cpp)
add_executable(test_thread test_thread.cpp)
add_executable(test_util test_util.cpp)
add_executable(test_fiber test_fiber.cpp)

target_link_libraries(test_log sylar yaml-cpp)
target_link_libraries(test_config sylar yaml-cpp)
target_link_libraries(test_thread sylar yaml-cpp pthread)
target_link_libraries(test_util sylar yaml-cpp pthread)
target_link_libraries(test_fiber sylar yaml-cpp pthread)
