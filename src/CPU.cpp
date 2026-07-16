//
// Created by arthu on 16/07/2026.
//
#include "CPU.h"
#include <fstream>
#include <iostream>
#include <cstring>

namespace MemoryMap
{
	constexpr uint16_t MEMORY_SIZE = 0xFFFF;
}

CPU::CPU()
	:	m_aMemory{0}
{
}

int CPU::LoadRom( const char* sROMPath )
{
	std::string sRomPath = std::string( PATH_ROMS ) + sROMPath;
	std::ifstream file( sRomPath,std::ios::binary | std::ios::in | std::ios::ate );
	if( file.is_open() )
	{
		std::streamsize size = file.tellg();
		char* memblock = nullptr;
		std::streamsize bytesRead = 0;
		try
		{
			if( size <= 0 || size > MemoryMap::MEMORY_SIZE )
				throw std::runtime_error( "SIZE_INVALID" );

			memblock = new char[ size ];
			file.seekg( 0,std::ios::beg );
			file.read( memblock,size );
			file.close();

			bytesRead = file.gcount();
			if( bytesRead != size )
				throw std::runtime_error( "READ_SIZE_NOT_CONFORM" );
		}
		catch ( std::runtime_error e )
		{
			delete[] memblock;
			std::cout << "ERROR::LOADING_ROM_" << e.what() << std::endl;
			return -1;
		}

		memcpy( m_aMemory, memblock, size );
		delete[] memblock;
	}
	else
	{
		std::cerr << "ERROR::ROM::LOADING::FILE_NOT_FOUND " << sRomPath  << std::endl;
		return -1;
	}
	return 0;
}
