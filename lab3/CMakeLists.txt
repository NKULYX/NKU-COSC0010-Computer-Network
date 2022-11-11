cmake_minimum_required(VERSION 3.21)
project(lab3)

include_directories(include)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_EXE_LINKER_FLAGS -static)

link_libraries(ws2_32 wsock32)

add_executable(lab3 main.cpp)
add_executable(UtilTest test/UtilTest.cpp)
add_executable(ThreadTest test/ThreadTest.cpp)
add_executable(GetFilesTest test/GetFilesTest.cpp)
add_executable(client_3_1 3-1/client.cpp)
add_executable(server_3_1 3-1/server.cpp)

