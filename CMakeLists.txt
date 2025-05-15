cmake_minimum_required(VERSION 3.31)
project(untitled10 C)

set(CMAKE_C_STANDARD 11)

add_executable(untitled10 main.c
        command_utils.h
        process_utils.h
        rlimit_utils.h
        signal_utils.h
        file_utils.h
        rlimit_utils.c
        command_utils.c
        process_utils.c
        signal_utils.c
        file_utils.c)
