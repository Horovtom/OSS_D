cmake_minimum_required(VERSION 3.6)
project(D)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES client.cpp route_cfg_parser.h route_cfg_parser.c)
add_executable(D ${SOURCE_FILES})
set(SOURCE_FILES_CFG_TEST test_cfg_parser.c route_cfg_parser.h route_cfg_parser.c)
add_executable(test_cfg_parser ${SOURCE_FILES_CFG_TEST})