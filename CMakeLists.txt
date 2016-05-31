cmake_minimum_required(VERSION 2.4)
PROJECT(snowbar)
#SET (CMAKE_BUILD_TYPE Release)
SET (CMAKE_BUILD_TYPE Debug)



IF(WIN32)
ELSE(WIN32)
set(CMAKE_CXX_FLAGS "-g -Wall -std=c++0x")
ENDIF(WIN32)


INCLUDE_DIRECTORIES(
json
json/include
xml/include
filewatcher/include
snowbar/include
)

SET (LIBRARY_TYPE STATIC)



ADD_LIBRARY(jsoncpp
json/source/json_reader.cpp
json/source/json_value.cpp
json/source/json_writer.cpp
json/json/json_serialize.hpp
)



IF(WIN32)
ADD_LIBRARY(filewatcher
filewatcher/source/FileWatcher.cpp
filewatcher/source/FileWatcherWin32.cpp
)
ELSE(WIN32)
ADD_LIBRARY(filewatcher
filewatcher/source/FileWatcher.cpp
filewatcher/source/FileWatcherLinux.cpp
)
ENDIF(WIN32)





ADD_EXECUTABLE(snowbar
snowbar/source/main.cpp
snowbar/source/socketserver.cpp
snowbar/include/socketserver.h
snowbar/source/simplesocket.cpp
snowbar/source/ipv4addr.cpp
snowbar/source/sha-1.c
snowbar/source/base64-decode.c
snowbar/source/icsocket.cpp
xml/source/SimpleXML.cpp
xml/include/SimpleXML.hpp
)


IF(WIN32)
TARGET_LINK_LIBRARIES(snowbar filewatcher ws2_32.lib)
ELSE(WIN32)
TARGET_LINK_LIBRARIES(snowbar filewatcher z rt)
ENDIF(WIN32)




ADD_DEFINITIONS(-D NOTHING_To_DEFINE_FOR_NOW )








