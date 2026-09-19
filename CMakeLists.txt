cmake_minimum_required(VERSION 3.13)

project(codecrafters-dns-server)

include_directories(
    inc
)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

file(GLOB TESTS_FILES 
    src/task.c
    src/utils.c
    src/cir_buffer.c
    src/sync_cir_buffer.c
    tests/*
)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(dns-server ${SOURCE_FILES})
add_executable(test-runner ${TESTS_FILES})
