#include <fstream>
#include <iosfwd>
#include <stdexcept>

#include "Disassembler.h"
#include <iostream>

#include "CPU.h"
#ifdef LEAK_DETECTOR
	#include <vld.h>
	#define ENABLE_GLOBAL_LEAK_DETECTION() VLDGlobalEnable()
	#define DISABLE_GLOBAL_LEAK_DETECTION() VLDGlobalDisable()

	#define ENABLE_SPECIFIC_LEAK_DETECTION() VLDEnable()
	#define DISABLE_SPECIFIC_LEAK_DETECTION() VLDDisable()
#else
	#define ENABLE_GLOBAL_LEAK_DETECTION() ((void)0)
	#define DISABLE_GLOBAL_LEAK_DETECTION() ((void)0)

	#define ENABLE_SPECIFIC_LEAK_DETECTION() ((void)0)
	#define DISABLE_SPECIFIC_LEAK_DETECTION() ((void)0)
#endif

int main( int argc, char *argv[] )
{
	Disassembler::m_oCPU;
	if ( argc <= 1 || Disassembler::m_oCPU.LoadRom( argv[ 1 ] ) != 0 )
		return -1;

	Disassembler m_oDisassembler;
	Disassembler::Disassemble_ROM( argv[1] );
	return 0;
}
