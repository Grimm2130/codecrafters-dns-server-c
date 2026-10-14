cmake_minimum_required(VERSION 3.13)

add_compile_options("-g")

project(codecrafters-dns-server)

include_directories(
    inc
)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

file(GLOB TESTS_FILES 
    src/task.c
    src/bitops.c
    src/utils.c
    src/cir_buffer.c
    src/sync_cir_buffer.c
    src/serializer.c
    src/network_serializer.c
    src/file_io.c
    src/logger.c
    tests/*
)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(dns-server ${SOURCE_FILES})
add_executable(dns-server-resolver ${SOURCE_FILES})
add_executable(test-runner ${TESTS_FILES})

if("$ENV{USER}" STREQUAL "chimaochiagha")
    target_compile_definitions(dns-server PUBLIC "-DLOCAL_BUILD=1")
    message("Local Run!!!")
endif()
target_compile_definitions(dns-server-resolver PUBLIC "-DRESOLVER=1")
