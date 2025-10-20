cmake_minimum_required(VERSION 3.22)

file(GLOB SOURCES "src/*.c")

project(kkd16-memory-allocator VERSION 1.0 DESCRIPTION "Memory Allocator" LANGUAGES C)
add_executable(main ${SOURCES})

include_directories(include)
