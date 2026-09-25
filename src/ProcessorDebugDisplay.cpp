//
// Created by arthu on 23/09/2026.
//

#include "ProcessorDebugDisplay.h"

#include <chrono>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "CPU.h"

void ProcessorDebugDisplay::Update()
{
	auto start = std::chrono::high_resolution_clock::now();

	glfwPollEvents();
	static double iDurationMs;
	char titleBuffer[ 128 ];
	std::snprintf( titleBuffer,sizeof( titleBuffer ),"Processor (%.2f ms)###ProcDebugDisplayWindow",iDurationMs );
	if( ImGui::Begin( titleBuffer,nullptr ) )
	{
		_DrawFlagsState();
		_DrawRegistersState();
	}

	ImGui::End();

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
}

void ProcessorDebugDisplay::_DrawFlagsState()
{

}

void ProcessorDebugDisplay::_DrawRegistersState()
{
}
