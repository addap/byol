cmake_minimum_required(VERSION 3.12)
project(byol C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(parser
        builtins.c
        lval.c
        lval.h
        macros.h
        mpc.c
        mpc.h
        parser.c
        builtins.h parser.h)

target_link_libraries(parser readline)
