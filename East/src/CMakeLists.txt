cmake_minimum_required(VERSION 3.26)
#project(East VERSION 1.0)

# set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function")
set(LIBRARY_NAME East)

aux_source_directory(. DIR_LIB_SRCS)

add_library(${LIBRARY_NAME} SHARED ${DIR_LIB_SRCS})

target_include_directories(${LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/East/include)