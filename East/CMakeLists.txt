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
    src/daemon.cc
    src/env.cc
    http/Http.cc
    # http/http11_parser.rl.cc 
    # http/httpclient_parser.rl.cc
    http/HttpParser.cc
    http/HttpSession.cc
    http/HttpConnection.cc
    src/Hook.cc
    src/Stream.cc
    src/SocketStream.cc
    http/HttpServer.cc
    http/Servlet.cc)
#file(GLOB_RECURSE SRC_FILES "src/*.cc")

function(ragelmaker src_rl outputlist outputdir)
    #Create a custom build step that will call ragel on the provided src_rl file.
    #The output .cpp file will be appended to the variable name passed in outputlist.

    get_filename_component(src_file ${src_rl} NAME_WE)

    set(rl_out ${outputdir}/${src_file}.rl.cc)

    #adding to the list inside a function takes special care, we cannot use list(APPEND...)
    #because the results are local scope only
    set(${outputlist} ${${outputlist}} ${rl_out} PARENT_SCOPE)

    #Warning: The " -S -M -l -C -T0  --error-format=msvc" are added to match existing window invocation
    #we might want something different for mac and linux
    add_custom_command(
        OUTPUT ${rl_out}
        COMMAND cd ${outputdir}
        COMMAND ragel ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl} -o ${rl_out} -l -C -G2  --error-format=msvc
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${src_rl}
        )
    set_source_files_properties(${rl_out} PROPERTIES GENERATED TRUE)
endfunction(ragelmaker)

ragelmaker(http/http11_parser.rl DIR_LIB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/http)
ragelmaker(http/httpclient_parser.rl DIR_LIB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/http)
ragelmaker(src/uri.rl DIR_LIB_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src)


add_library(${LIBRARY_NAME} SHARED ${DIR_LIB_SRCS})
target_link_libraries(${LIBRARY_NAME} PRIVATE dl pthread)
#add_library(${LIBRARY_NAME} SHARED ${SRC_FILES})

target_include_directories(${LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
#target_link_libraries(${LIBRARY_NAME} PUBLIC yaml-cpp::yaml-cpp)