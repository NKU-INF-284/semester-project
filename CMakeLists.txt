cmake_minimum_required(VERSION 3.23)
project(semester_project)

set(CMAKE_CXX_STANDARD 14)

include_directories(src)

add_executable(semester_project
        src/main.cpp
        src/server.cpp
        src/server.hpp src/echo_server.cpp src/echo_server.hpp src/message_server.cpp src/message_server.hpp)
