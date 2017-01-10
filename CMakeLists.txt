cmake_minimum_required(VERSION 3.6)
project(D)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(client_threads Thread_architecture/client.cpp Thread_architecture route_cfg_parser.c)
