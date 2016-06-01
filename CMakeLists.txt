cmake_minimum_required(VERSION 2.4)
PROJECT(snowbar)
#SET (CMAKE_BUILD_TYPE Release)
SET (CMAKE_BUILD_TYPE Debug)

enable_testing()


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
gtest
gtest/include
)

SET (LIBRARY_TYPE STATIC)

ADD_LIBRARY(GTest
gtest/src/gtest-all.cc
)


ADD_LIBRARY(GTestMain
gtest/src/gtest-all.cc
gtest/src/gtest_main.cc
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

add_executable(testFunctions
gtest/src/gtest-all.cc
gtest/src/gtest_main.cc
test/Functions.cpp
test/Functions_tests.cpp
)

TARGET_LINK_LIBRARIES(testFunctions pthread)


add_test(test1 testFunctions)

#add_custom_command(TARGET testFunctions
#                   POST_BUILD
#                   COMMAND  ctest --output-on-failure 1>&2 )

IF(WIN32)
ELSE(WIN32)
add_custom_command(TARGET testFunctions
                   POST_BUILD
                   COMMAND testFunctions  1>&2 )
ENDIF(WIN32)



add_custom_target(unittest  ${PROJECT_NAME})

ADD_DEFINITIONS(-D NOTHING_To_DEFINE_FOR_NOW )








