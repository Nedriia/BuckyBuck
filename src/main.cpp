#include <fstream>
#include <iosfwd>
#include <stdexcept>

#include "Disassembler.h"
#include <iostream>
#include "Display.h"
#include "DebugInfosDisplay.h"

#include "CPU.h"

int Quit()
{
#ifdef DEBUG_INFO
	DebugInfosDisplay::GetInstance()->Destroy();
#endif

	Display::GetInstance()->DestroyWindow();
	CPU::GetInstance()->DestroyInstance();
	
	return -1;
}

int main( int argc, char *argv[] )
{
	Display* m_pDisplayInstance = Display::GetInstance();
	CPU*	m_pCPUInstance		= CPU::GetInstance();

	if( m_pDisplayInstance->Init() != 0 )
	{
		Quit();
		return -1;
	}

	if ( argc <= 1 || m_pCPUInstance->LoadRom( argv[ 1 ] ) != 0 )
		return -1;

	Disassembler m_oDisassembler;
	Disassembler::Disassemble_ROM( argv[1] );

	bool quit = false;
	while( !quit )
	{
		m_pDisplayInstance->Update( quit );

	}

	Quit();
	return 0;
}
