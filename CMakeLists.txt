cmake_minimum_required(VERSION 3.8)
project(Ping)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp MyPing.cpp MyPing.h)
add_executable(Ping ${SOURCE_FILES})
target_link_libraries(Ping wsock32 ws2_32)