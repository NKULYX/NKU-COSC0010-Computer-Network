cmake_minimum_required(VERSION 3.21)
project(CS)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXE_LINKER_FLAGS -static)

link_libraries(ws2_32 wsock32)

add_executable(server CS/server.cpp CS/util.h)
add_executable(client CS/client.cpp CS/util.h)
add_executable(P2P_client P2P/client.c)


