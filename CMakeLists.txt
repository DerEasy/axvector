cmake_minimum_required(VERSION 3.26)
project(axvector C)

add_executable(axvector main.c
        axvector.c
        axvector.h)

target_compile_options(axvector PRIVATE -Wall -Wpedantic -std=c17 -O3)

#target_link_libraries(axvector PRIVATE ubsan)
