cmake_minimum_required(VERSION 3.16)
project(esetvm2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "-O2 -g")

set(SOURCES main.c esetvm2hdr.c esetvm2decode.c esetvm2.c)

add_executable(esetvm2 ${SOURCES})

target_link_libraries(esetvm2 -lpthread)
