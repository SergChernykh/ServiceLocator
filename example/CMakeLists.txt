cmake_minimum_required(VERSION 3.5)

project(ServiceLocatorExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDES ${CMAKE_SOURCE_DIR}/../include)

set(HEADERS ${INCLUDES}/servicelocator.hpp)
set(SOURCES main.cpp)

set(TARGET ServiceLocatorExample)

add_executable(${TARGET} ${HEADERS} ${SOURCES})
target_include_directories(${TARGET} PRIVATE ${INCLUDES})
