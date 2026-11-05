//
// Created by arthu on 05/11/2026.
//

#include "VRAMViewer.h"
#include <chrono>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "CPU.h"

void VRAMViewer::Update()
{
	auto start = std::chrono::high_resolution_clock::now();

	glfwPollEvents();
	static double iDurationMs;

	char titleBuffer[ 128 ];
	std::snprintf( titleBuffer,sizeof( titleBuffer ),"VRAM Viewer (%.2f ms)###VRAMDisplayWindow",iDurationMs );
	if( ImGui::Begin( titleBuffer,nullptr ) )
	{
	}
	ImGui::End();

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
}