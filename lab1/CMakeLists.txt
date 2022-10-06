cmake_minimum_required(VERSION 3.21)
project(lab1 C)

set(CMAKE_C_STANDARD 11)

link_libraries(ws2_32 wsock32)

add_executable(lab1 main.c)