cmake_minimum_required(VERSION 3.5)

project(vino)

set(CMAKE_C_FILES "{CMAKE_C_FLAGS}")

aux_source_directory(src DIR_SRCS)
# set(SOURCE_FILES src/vino.c src/socketlib.c src/rio.c src/util.c src/vn_epoll.c src/vn_request.c)

add_executable(vino ${DIR_SRCS})