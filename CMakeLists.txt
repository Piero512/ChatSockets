cmake_minimum_required(VERSION 3.14)
project(SocketExample C)

set(CMAKE_C_STANDARD 99)

find_package(Threads REQUIRED)

add_executable(Server server.c common.c common.h)

add_executable(Client client.c common.c common.h)


add_library(binn libs/binn.c libs/binn.h)

add_library(StsQueue libs/sts_queue.c libs/sts_queue.h)


target_link_libraries(Server Threads::Threads StsQueue binn)
target_link_libraries(Client Threads::Threads StsQueue binn)