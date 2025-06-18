cmake_minimum_required(VERSION 3.22)
project(East VERSION 1.0)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -fPIC -rdynamic -g -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -ldl")
set(CMAKE_C_FLAGS "$ENV{CXXFLAGS} -fPIC -rdynamic -g -std=c11 -Wall -Wno-deprecated -Werror -Wno-unused-function -ldl")

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

find_library(PTHREAD pthread)

file(GLOB_RECURSE ALL_SOURCE_FILES East/src/*.cc East/include/*.h tests/*.cc)
add_custom_target(
    format
    COMMAND clang-format -i ${ALL_SOURCE_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

include_directories(${yaml-cpp_SOURCE_DIR}/include)
add_subdirectory(East)

set(LIBS 
        East
        pthread
        dl
        yaml-cpp)
#link_directories(${PROJECT_SOURCE_DIR}/bin)

#ADD_LIBRARY(East SHARED ${LIB_SRC})
add_executable(test tests/test.cc)
add_dependencies(test East)
target_link_libraries(test East yaml-cpp)

add_executable(test_config tests/test_config.cc)
target_link_libraries(test_config "${LIBS}")

add_executable(test_thread tests/test_thread.cc)
target_link_libraries(test_thread "${LIBS}")

add_executable(test_util tests/test_util.cc)
target_link_libraries(test_util "${LIBS}")

add_executable(test_fiber tests/test_fiber.cc)
target_link_libraries(test_fiber "${LIBS}")

add_executable(test_scheduler tests/test_scheduler.cc)
target_link_libraries(test_scheduler "${LIBS}")

add_executable(test_iomanager tests/test_iomanager.cc)
target_link_libraries(test_iomanager "${LIBS}")

add_executable(test_hook tests/test_hook.cc)
target_link_libraries(test_hook "${LIBS}")

add_executable(test_address tests/test_address.cc)
target_link_libraries(test_address "${LIBS}")

add_executable(test_socket tests/test_socket.cc)
target_link_libraries(test_socket "${LIBS}")

add_executable(test_bytearray tests/test_bytearray.cc)
target_link_libraries(test_bytearray "${LIBS}")

add_executable(test_http tests/test_http.cc)
target_link_libraries(test_http "${LIBS}")

add_executable(test_http_parser tests/test_http_parser.cc)
target_link_libraries(test_http_parser "${LIBS}")

add_executable(test_tcp_server tests/test_tcp_server.cc)
target_link_libraries(test_tcp_server "${LIBS}")

add_executable(echo_server examples/echo_server.cc)
target_link_libraries(echo_server "${LIBS}")

add_executable(test_http_server tests/test_http_server.cc)
target_link_libraries(test_http_server "${LIBS}")

add_executable(test_http_connection tests/test_http_connection.cc)
target_link_libraries(test_http_connection "${LIBS}")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)