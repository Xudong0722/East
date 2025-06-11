cmake_minimum_required(VERSION 3.22)
project(East VERSION 1.0)


find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_library(PTHREAD pthread)
# set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++17 -W  all -Wno-deprecated -Werror -Wno-unused-function")
set(LIBRARY_NAME East)

# aux_source_directory(src DIR_LIB_SRCS)
set(DIR_LIB_SRCS 
    src/Address.cc
    src/ByteArray.cc 
    src/Config.cc 
    src/FdManager.cc 
    src/Fiber.cc 
    src/IOManager.cc 
    src/LogAppender.cc 
    src/LogEvent.cc 
    src/LogFormatter.cc 
    src/Logger.cc 
    src/Mutex.cc 
    src/Scheduler.cc 
    src/Socket.cc 
    src/Thread.cc 
    src/Timer.cc 
    src/util.cc 
    src/TcpServer.cc
    http/Http.cc
    http/http11_parser.rl.cc 
    http/httpclient_parser.rl.cc
    http/HttpParser.cc
    http/HttpSession.cc
    src/Hook.cc
    src/Stream.cc
    src/SocketStream.cc
    http/HttpServer.cc)
#file(GLOB_RECURSE SRC_FILES "src/*.cc")


add_library(${LIBRARY_NAME} SHARED ${DIR_LIB_SRCS})
target_link_libraries(${LIBRARY_NAME} PRIVATE dl pthread)
#add_library(${LIBRARY_NAME} SHARED ${SRC_FILES})

target_include_directories(${LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
#target_link_libraries(${LIBRARY_NAME} PUBLIC yaml-cpp::yaml-cpp)