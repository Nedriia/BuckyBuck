cmake_minimum_required(VERSION 3.20)
project(BuckyBuck)

set( CMAKE_CXX_STANDARD 20)

set( PROJECT_DIR    "${CMAKE_CURRENT_SOURCE_DIR}" )
set( VENDOR_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/vendor" )
set( LIBS_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/librairies" )
set( SRC_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/src" )
set( IMGUI_DIR      "${VENDOR_DIR}/imgui")
set( HEX_DIR        "${VENDOR_DIR}/HexEditor/src")

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

set(PROJECT_SOURCES
        ${SRC_DIR}/main.cpp
        ${SRC_DIR}/Disassembler.cpp
        ${SRC_DIR}/CPU.cpp
        ${SRC_DIR}/Display.cpp
        ${SRC_DIR}/DebugInfosDisplay.cpp
)

set( HEXEDITOR_SOURCES
        ${HEX_DIR}/Buffer.cpp
        ${HEX_DIR}/HexEditor.cpp
        ${HEX_DIR}/HexEditor_ImGUI.cpp
)

add_executable(${PROJECT_NAME}
        ${IMGUI_SOURCES}
        ${PROJECT_SOURCES}
        ${HEXEDITOR_SOURCES}

        ${VENDOR_DIR}/glad.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${IMGUI_DIR}/backends
        ${IMGUI_DIR}
        ${HEX_DIR}
        ${LIBS_DIR}/Include
)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(${VENDOR_DIR}/glfw ${CMAKE_BINARY_DIR}/glfw_build)

find_package(OpenGL REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        OpenGL::GL
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        PATH_ROMS="${CMAKE_CURRENT_SOURCE_DIR}/Roms/"
        DISASSM_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Disassembly/"
)

option( LEAK_DETECTOR_ENABLE "Enable leak detector" ON )
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
