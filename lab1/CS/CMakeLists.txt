cmake_minimum_required(VERSION 3.21)
project(CS)

set(CMAKE_CXX_STANDARD 14)

link_libraries(ws2_32 wsock32)

add_executable(CS main.cpp)
add_executable(server server.cpp util.h)
add_executable(client client.cpp util.h)
