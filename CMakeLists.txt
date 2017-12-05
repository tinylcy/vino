cmake_minimum_required(VERSION 3.5)

project(vino)

set(CMAKE_C_FILES "-Wall")

file(GLOB SRCS src/*.c)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_definitions(-DDEBUG)
endif()

add_executable(vino ${SRCS})


######## PREVIOUS VERSION ########

# cmake_minimum_required(VERSION 3.5)
# project(vino) 
# set(CMAKE_C_FILES "{CMAKE_C_FLAGS}")
# aux_source_directory(src DIR_SRCS)
# add_executable(vino ${DIR_SRCS})