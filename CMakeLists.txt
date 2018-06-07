cmake_minimum_required(VERSION 3.10)
project(transmitter C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "-pthread")

add_executable(transmitter radio.c radio.h err.c err.h transmitter.c buffer_handler.c buffer_handler.h transmitter.h missing_handler.c missing_handler.h validate.c validate.h)