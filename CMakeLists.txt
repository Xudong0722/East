cmake_minimum_required(VERSION 3.26)
project(East VERSION 1.0)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function")

# SET(LIB_SRC 
#     East/src/log.cc
#     East/src/util.cc)

#include_directories(${PROJECT_SOURCE_DIR}/East/thirdparty/include)

include(FetchContent)
FetchContent_Declare(
    yaml-cpp 
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git 
    GIT_TAG master
)
FetchContent_MakeAvailable(yaml-cpp)

include_directories(${yaml-cpp_SOURCE_DIR}/include)
add_subdirectory(East)

#link_directories(${PROJECT_SOURCE_DIR}/bin)

#ADD_LIBRARY(East SHARED ${LIB_SRC})
add_executable(test tests/test.cc)
add_dependencies(test East)
target_link_libraries(test East yaml-cpp)

add_executable(test_config tests/test_config.cc)
#add_dependencies(test_config East)
#add_dependencies(test_config yaml-cpp)
target_link_libraries(test_config East)
target_link_libraries(test_config yaml-cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)