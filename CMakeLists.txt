cmake_minimum_required(VERSION 3.20)
project(BuckyBuck)

set( CMAKE_CXX_STANDARD 20)

set( PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" )
set( VENDOR_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/vendor" )
set( LIBS_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/librairies" )
set( SRC_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/src" )

add_executable( ${PROJECT_NAME}
                ${SRC_DIR}/main.cpp
                ${SRC_DIR}/Disassembler.cpp
                ${SRC_DIR}/Disassembler.h
)