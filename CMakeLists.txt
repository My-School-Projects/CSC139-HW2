cmake_minimum_required(VERSION 3.13)
project(CSC133A2 C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "-O0 -Wall -Wextra -pthread")

add_executable(CSC133HW2 MTFindMin.c)
