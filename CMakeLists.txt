cmake_minimum_required(VERSION 3.26)
project(East VERSION 1.0)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function")

# SET(LIB_SRC 
#     East/src/log.cc
#     East/src/util.cc)

add_subdirectory(East/src)

#ADD_LIBRARY(East SHARED ${LIB_SRC})
add_executable(test tests/test.cc)
add_dependencies(test East)
target_link_libraries(test East)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)