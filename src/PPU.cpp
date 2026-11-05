//
// Created by arthu on 05/11/2026.
//

#include "PPU.h"
#include <cstdio>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Display.h"

PPU* PPU::m_pSingleton = nullptr;

PPU::PPU()
{
	m_pPixels[ 160 ][ 144 ] = { 0 };
}

void PPU::Update( const int iCycles )
{

}