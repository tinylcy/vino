cmake_minimum_required (VERSION 2.6)

project (tinyhttpd)

file(GLOB SOURCES "*.c")

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-W -Wall -pthread")

add_executable(tinyhttpd ${SOURCES})