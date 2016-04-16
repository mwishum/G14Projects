cmake_minimum_required(VERSION 3.3)
project(G14Project1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/packets.cpp
        src/packets.h
        src/project.h
        src/Project2.cpp
        src/FileManager.cpp
        src/FileManager.h
        src/Sockets.h
        src/Sockets.cpp
        src/Gremlin.cpp
        src/Gremlin.h
        src/side/Server.h
        src/side/Client.h
        README.md)

add_executable(G14Project1 ${SOURCE_FILES})