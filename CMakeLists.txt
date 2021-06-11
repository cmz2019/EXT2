cmake_minimum_required(VERSION 3.17)
project(EXT2 C)

set(CMAKE_C_STANDARD 99)

add_executable(EXT2 main.c main.h init.h init.c)