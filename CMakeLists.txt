cmake_minimum_required(VERSION 3.20)
project(BuckyBuck)

set( CMAKE_CXX_STANDARD 20)

set( PROJECT_DIR    "${CMAKE_CURRENT_SOURCE_DIR}" )
set( VENDOR_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/vendor" )
set( LIBS_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/librairies" )
set( SRC_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/src" )

add_executable( ${PROJECT_NAME}
                ${SRC_DIR}/main.cpp
                ${SRC_DIR}/Disassembler.cpp
                ${SRC_DIR}/Disassembler.h
                ${SRC_DIR}/CPU.cpp
                ${SRC_DIR}/CPU.h
                ${SRC_DIR}/HexEditor.cpp
                ${SRC_DIR}/HexEditor.h
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        PATH_ROMS="${CMAKE_CURRENT_SOURCE_DIR}/Roms/"
        DISASSM_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Disassembly/"
)

option( LEAK_DETECTOR_ENABLE "Enable leak detector" OFF )
if( LEAK_DETECTOR_ENABLE )
    target_compile_definitions( ${PROJECT_NAME} PRIVATE LEAK_DETECTOR )

    set(VLD_PATH "C:/Program Files (x86)/Visual Leak Detector")

    target_include_directories(${PROJECT_NAME} PRIVATE
            ${VLD_PATH}/include
    )

    target_link_libraries(${PROJECT_NAME}
            PRIVATE
            "${VLD_PATH}/lib/Win64/vld.lib"
    )
endif()