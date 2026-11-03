#include <fstream>
#include <iosfwd>
#include <stdexcept>

#include "Disassembler.h"
#include <iostream>
#include "Display.h"
#include "DebugInfosDisplay.h"
#include "TimeManager.h"

#include "CPU.h"
#include "Timer.h"

int Quit()
{
#ifdef DEBUG_INFO
	DebugInfosDisplay::GetInstance()->Destroy();
#endif

	Display::GetInstance()->DestroyWindow();
	CPU::GetInstance()->DestroyInstance();
	Timer::GetInstance()->DestroyInstance();

	return -1;
}

int main( int argc, char *argv[] )
{
	Display*	m_pDisplayInstance = Display::GetInstance();
	CPU*		m_pCPUInstance		= CPU::GetInstance();

	if( m_pDisplayInstance->Init() != 0 )
	{
		Quit();
		return -1;
	}

	if ( argc <= 1 || m_pCPUInstance->LoadRom( argv[ 1 ] ) != 0 )
	{
		Quit();
		return -1;
	}

	Disassembler m_oDisassembler;
	Disassembler::Disassemble_ROM( argv[1] );

	bool quit = false;
	while( !quit )
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

		m_pCPUInstance->EmulateCycle();
		m_pDisplayInstance->Update( quit );

		TimeManager::HandleTime( start );
	}

	Quit();
	return 0;
}
