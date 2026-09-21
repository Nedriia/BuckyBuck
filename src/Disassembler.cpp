//
// Created by arthu on 09/07/2026.
//

#include <iostream>
#include "Disassembler.h"

#include <filesystem>
#include <fstream>
#include <stdarg.h>
#include <vector>
#include "CPU.h"

static int g_iCounter = 0;
CPU* Disassembler::m_pCPU = nullptr;

Disassembler::Disassembler()
{
	Init();
}

Disassembler::~Disassembler()
{
}

void Disassembler::Init()
{
	m_pCPU = CPU::GetInstance();
}

void Disassembler::Disassemble_ROM( const char* sRomPath )
{
	std::filesystem::path outputPath = DISASSM_DIR / static_cast<std::filesystem::path>( sRomPath ).filename();
	outputPath.replace_extension( ".asm" );

	if ( exists( outputPath ) == false )
	{
		std::fstream file;
		file.open( outputPath.string(), std::ofstream::out );

		if( file.is_open() )
		{
			g_iCounter = 0;
			uint8_t iLengthIncrease = 0;
			for ( uint16_t iPC = 0x0000; iPC < 0x7FFF; )
			{
				if ( iPC >= 0x104 && iPC <= 0x14F )
				{
					DecryptCartridge(file );
					iPC = 0x150;
					continue;
				}

				_WriteInstruction( file, iPC, "",&iLengthIncrease );
				iPC += iLengthIncrease;
			}
		}
	}
}

std::string Disassembler::Format( const char* sFormat, ... )
{
	va_list args;
	va_start(args, sFormat);
	size_t len = std::vsnprintf(nullptr, 0, sFormat, args);
	va_end(args);

	std::vector<char> vec(len + 1);
	va_start(args, sFormat);
	std::vsnprintf(&vec[0], len + 1, sFormat, args);
	va_end(args);

	return &vec[0];
}

void Disassembler::DecryptCartridge( std::fstream& file )
{
	_WriteInstruction( file, 0x104, ";Nintendo Logo" );
	_WriteInstruction( file, 0x134, ";Title");
	_WriteInstruction( file, 0x13F, ";Manufacturer code");
	_WriteInstruction( file, 0x143, ";CGB Flag");
	_WriteInstruction( file, 0x144, ";New Licence Code");
	_WriteInstruction( file, 0x146, ";SGB Flag");
	_WriteInstruction( file, 0x147, ";Cartridge Type");
	_WriteInstruction( file, 0x148, ";ROM size");
	_WriteInstruction( file, 0x149, ";RAM size");
	_WriteInstruction( file, 0x14A, ";Destination code");
	_WriteInstruction( file, 0x14B, ";Old Licence code");
	_WriteInstruction( file, 0x14C, ";Mask ROM version number");
	_WriteInstruction( file, 0x14D, ";Header Checksum");
	_WriteInstruction( file, 0x14E, ";Global Checksum");
}

void Disassembler::DecryptIORange( std::fstream& file )
{
	_WriteInstruction( file, 0xFF00, ";Joypad");
	_WriteInstruction( file, 0xFF01, ";Serial data");
	_WriteInstruction( file, 0xFF02, ";Serial control");
	_WriteInstruction( file, 0xFF03, "");
	_WriteInstruction( file, 0xFF04, ";Divider");
	_WriteInstruction( file, 0xFF05, ";Timer counter");
	_WriteInstruction( file, 0xFF06, ";Timer modulo");
	_WriteInstruction( file, 0xFF07, ";Timer ctrl");
	_WriteInstruction( file, 0xFF08, "");
	_WriteInstruction( file, 0xFF0F, ";Int flag");
	for ( int i = 0xFF10; i <= 0xFF26; ++i )
		_WriteInstruction( file, i, Format( "NR%i", i ).c_str());
	_WriteInstruction( file, 0xFF27, "");
	_WriteInstruction( file, 0xFF30, ";Wave pattern");
	_WriteInstruction( file, 0xFF40, ";lcd ctrl");
	_WriteInstruction( file, 0xFF41, ";lcd stat");
	_WriteInstruction( file, 0xFF42, ";scroll Y");
	_WriteInstruction( file, 0xFF43, ";scroll X");
	_WriteInstruction( file, 0xFF44, ";LY");
	_WriteInstruction( file, 0xFF45, ";LYC");
	_WriteInstruction( file, 0xFF46, ";OAM DMA");
	_WriteInstruction( file, 0xFF47, ";bg pal");
	_WriteInstruction( file, 0xFF48, ";obj pal 0");
	_WriteInstruction( file, 0xFF49, ";obj pal 1");
	_WriteInstruction( file, 0xFF4A, ";win Y");
	_WriteInstruction( file, 0xFF4B, ";win X");
	_WriteInstruction( file, 0xFF4D, ";speed switch");
	_WriteInstruction( file, 0xFF4F, ";vram bank");
	_WriteInstruction( file, 0xFF50, ";disable bootrom");
	_WriteInstruction( file, 0xFF51, ";hdma src hi");
	_WriteInstruction( file, 0xFF52, ";hdma src low");
	_WriteInstruction( file, 0xFF53, ";hdma dest hi");
	_WriteInstruction( file, 0xFF54, ";hdma dest low");
	_WriteInstruction( file, 0xFF55, ";hdma count");
	_WriteInstruction( file, 0xFF56, ";ir port");
	_WriteInstruction( file, 0xFF68, ";bg pal set");
	_WriteInstruction( file, 0xFF69, ";bg pal data");
	_WriteInstruction( file, 0xFF6A, ";obj pal sel");
	_WriteInstruction( file, 0xFF6B, ";obj pal data");
	_WriteInstruction( file, 0xFF70, ";wram bank");
	_WriteInstruction( file, 0xFF76, ";PCM34");
	_WriteInstruction( file, 0xFF77, ";PCM12");
}

void Disassembler::_WriteInstruction( std::fstream& file, const uint16_t iAdress, const char* sComment,uint8_t* iLengthIncrease /*= nullptr*/ )
{
	std::ostringstream oss;
	oss << Format( "%04X	", iAdress );
	
	switch ( iAdress )
	{
		case 0x104 :
		{
			int i = 0;
			while ( iAdress + i < 0x134 )
			{
				oss << Format (" %02X", m_pCPU->GetDataAtAdress( iAdress + i ) );
				++i;
				if ( i % 16 == 0 && iAdress + i != 0x134 )
					oss << Format ("\n%04X	", iAdress + i );
			}
			oss << sComment << "\n";
			file << oss.str();
			return;
		}
		case 0x134:
		{
			oss << " ";
			int i = 0;
			while ( iAdress + i < 0x143 )
			{
				if ( m_pCPU->GetDataAtAdress( iAdress + i ) == 0 )
					break;
				oss << Format ("%c",m_pCPU->GetDataAtAdress( iAdress + i ) );
				++i;
			}
			break;
		}
		case 0x143:
		{
			switch ( m_pCPU->GetDataAtAdress( iAdress ) )
			{
				case 0:
					oss << Format (" %02X		;DMG - classic gameboy",m_pCPU->GetDataAtAdress( iAdress ) );
					break;
				case 0x80:
					oss << Format (" %02X		;CGB - retro compat monochrome",m_pCPU->GetDataAtAdress( iAdress ) );
					break;
				case 0xC0:
					oss << Format (" %02X		;CGB - only",m_pCPU->GetDataAtAdress( iAdress ) );
					break;
			}
			break;
		}
		case 0x147:
		{
			std::string sText;
			switch ( m_pCPU->GetDataAtAdress( iAdress ) )
			{
				case 0x00: sText =  "ROM ONLY" ; break;
				case 0x01: sText =  "MBC1" ; break;
				case 0x02: sText =  "MBC1+RAM" ; break;
				case 0x03: sText =  "MBC1+RAM+BATTERY" ; break;
				case 0x05: sText =  "MBC2" ; break;
				case 0x06: sText =  "MBC2+BATTERY" ; break;
				case 0x08: sText =  "ROM+RAM" ; break;
				case 0x09: sText =  "ROM+RAM+BATTERY" ; break;
				case 0x0B: sText =  "MMM01" ; break;
				case 0x0C: sText =  "MMM01+RAM" ; break;
				case 0x0D: sText =  "MMM01+RAM+BATTERY" ; break;
				case 0x0F: sText =  "MBC3+TIMER+BATTERY" ; break;
				case 0x10: sText =  "MBC3+TIMER+RAM+BATTERY" ; break;
				case 0x11: sText =  "MBC3" ; break;
				case 0x12: sText =  "MBC3+RAM" ; break;
				case 0x13: sText =  "MBC3+RAM+BATTERY" ; break;
				case 0x19: sText =  "MBC5" ; break;
				case 0x1A: sText =  "MBC5+RAM" ; break;
				case 0x1B: sText =  "MBC5+RAM+BATTERY" ; break;
				case 0x1C: sText =  "MBC5+RUMBLE" ; break;
				case 0x1D: sText =  "MBC5+RUMBLE+RAM" ; break;
				case 0x1E: sText =  "MBC5+RUMBLE+RAM+BATTERY" ; break;
				case 0x20: sText =  "MBC6" ; break;
				case 0x22: sText =  "MBC7+SENSOR+RUMBLE+RAM+BATTERY" ; break;
				case 0xFC: sText =  "POCKET CAMERA" ; break;
				case 0xFD: sText =  "BANDAI TAMA5" ; break;
				case 0xFE: sText =  "HuC3" ; break;
				case 0xFF: sText =  "HuC1+RAM+BATTERY" ; break;
				default:   sText =  "Unknown Type" ; break;
			}
			oss << Format (" %02X		;%s",m_pCPU->GetDataAtAdress( iAdress ), sText.c_str() );
			break;
		}
		case 0x148:
		{
			std::string sText;
			switch ( m_pCPU->GetDataAtAdress( iAdress ) )
			{
				case 0x00: sText = "32 KiB, 2 (no banking)"; break;
				case 0x01: sText = "64 KiB, 4"; break;
				case 0x02: sText = "128 KiB, 8"; break;
				case 0x03: sText = "256 KiB, 16"; break;
				case 0x04: sText = "512 KiB, 32"; break;
				case 0x05: sText = "1 MiB, 64"; break;
				case 0x06: sText = "2 MiB, 128"; break;
				case 0x07: sText = "4 MiB, 256"; break;
				case 0x08: sText = "8 MiB, 512"; break;
				case 0x52: sText = "1.1 MiB, 72"; break;
				case 0x53: sText = "1.2 MiB, 80"; break;
				case 0x54: sText = "1.5 MiB, 96"; break;
				default:   sText = "Unknown Value"; break;
			}
			oss << Format (" %02X		;%s",m_pCPU->GetDataAtAdress( iAdress ), sText.c_str() );
			break;
		}
		case 0x149:
		{
			std::string sText;
			switch ( m_pCPU->GetDataAtAdress( iAdress ) )
			{
				case 0x00: sText =  "0, No RAM"; break;
				case 0x01: sText =  "-, Unused"; break;
				case 0x02: sText =  "8 KiB, 1 bank"; break;
				case 0x03: sText =  "32 KiB, 4 banks of 8 KiB each"; break;
				case 0x04: sText =  "128 KiB, 16 banks of 8 KiB each"; break;
				case 0x05: sText =  "64 KiB, 8 banks of 8 KiB each"; break;
				default:   sText =  "Unknown Code"; break;
			}
			oss << Format (" %02X		;%s",m_pCPU->GetDataAtAdress( iAdress ), sText.c_str() );
			break;
		}
		case 0x14A:
		{
			if ( m_pCPU->GetDataAtAdress( iAdress ) == 0 )
				oss << Format (" %02X		;Destination code : Japanese",m_pCPU->GetDataAtAdress( iAdress ) );
			else if ( m_pCPU->GetDataAtAdress( iAdress ) == 1 )
				oss << Format (" %02X		;Destination code : Overseas only",m_pCPU->GetDataAtAdress( iAdress ) );
			break;
		}
		case 0x14E:
		{
			oss << Format( " %02X %02X		",m_pCPU->GetDataAtAdress( iAdress ),m_pCPU->GetDataAtAdress( iAdress + 1 ) );
			break;
		}
		case 0x13F:
		case 0x144:
		case 0x146:
		case 0x14B:
		case 0x14C:
		case 0x14D:
		{
			oss << Format( " %02X	",m_pCPU->GetDataAtAdress( iAdress ) );
			break;
		}

		default:
			CPU::CPU_Instructions* pInstruction = CPU::m_aOpcodesTable[ m_pCPU->GetDataAtAdress( iAdress ) ];
			if( pInstruction != nullptr )
			{
				int i = 0;
				while( i < 4 )
				{
					if( iAdress + i < iAdress + pInstruction->m_iLength )
						oss << Format( " %02X",m_pCPU->GetDataAtAdress( iAdress + i ) );
					else
						oss << "	";
					++i;
				}
				oss << "		" << pInstruction->m_sMnemonic;

				g_iCounter += pInstruction->m_iDuration >> 2;
				oss << Format( "	;%i		%i",pInstruction->m_iDuration >> 2,g_iCounter );
				if( iLengthIncrease )
					*iLengthIncrease = pInstruction->m_iLength;
			}
			else
				oss << Format( " %02X	",m_pCPU->GetDataAtAdress( iAdress ) );

			break;
	}


	oss << "	" << sComment;
	file << oss.str() << "\n";
}