//
// Created by arthu on 09/07/2026.
//

#include <iostream>
#include "Disassembler.h"

#include <filesystem>
#include <fstream>
#include <stdarg.h>
#include <cstring>
#include <vector>
#include "CPU.h"


static constexpr uint8_t X_MASK = 0b11000000;
static constexpr uint8_t Y_MASK = 0b00111000;
static constexpr uint8_t P_MASK = 0b00110000;
static constexpr uint8_t Q_MASK = 0b00001000;
static constexpr uint8_t Z_MASK = 0b00000111;

static constexpr uint32_t r[ 8 ] = { 'B','C','D','E','H','L',' ','A' };
static constexpr std::string_view rp[ 4 ] = { "BC","DE","(HL)","SP" };
static constexpr std::string_view rp2[ 4 ] = { "BC","DE","(HL)","AF" };
static constexpr std::string_view cc[ 4 ] = { "NZ","Z","NC","C" };
static constexpr std::string_view alu[ 8 ] = { "ADD A,","ADC A,","SUB","SBC A,","AND","XOR","OR","CP" };
static constexpr std::string_view rot[ 8 ] = { "RLC","RRC","RL","RR","SLA","SRA","SWAP","SRL" };

Disassembler::CPU_Instructions* Disassembler::m_aOpcodesTable[ 256 ] = { nullptr };
Disassembler::CPU_Instructions* Disassembler::m_aExtendOpcodesTable[ 256 ] = { nullptr };

static int g_iCounter = 0;
CPU Disassembler::m_oCPU;

Disassembler::Disassembler()
{
	Init();
}

Disassembler::~Disassembler()
{
	for( int i = 0; i < 256; ++i )
	{
		if( m_aOpcodesTable[ i ] != nullptr )
			delete m_aOpcodesTable[ i ];
		if( m_aExtendOpcodesTable[ i ] != nullptr )
			delete m_aExtendOpcodesTable[ i ];
	}
}

void Disassembler::Init()
{
	_FillOpcodesTables();
}

void Disassembler::AddCPUInstruction( const uint8_t iIndex,const fct_opcode& fct,uint8_t iFlags,uint8_t iFlagSet1, uint8_t iFlagReset0, std::array<uint8_t,3> aValues,bool bExtent,const char* sMnemonic,... )
{
	if( m_aOpcodesTable[ iIndex ] != nullptr && !bExtent )
	{
		std::cout << "Opcode index already exist" << std::endl;
		return;
	}

	if( m_aExtendOpcodesTable[ iIndex ] != nullptr && bExtent )
	{
		std::cout << "Extend index already exist" << std::endl;
		return;
	}

	CPU_Instructions* pInstruction = new CPU_Instructions();

	va_list args;
	va_start( args,sMnemonic );
	vsnprintf( pInstruction->m_sMnemonic,32,sMnemonic,args );
	va_end( args );

	pInstruction->m_pFct = fct;
	pInstruction->m_aFlags = iFlags;
	pInstruction->m_aFlagSet_1 = iFlagSet1;
	pInstruction->m_aFlagReset_0 = iFlagReset0;
	pInstruction->m_iLength = aValues[0];
	pInstruction->m_iDuration = aValues[1];
	pInstruction->m_iConditionalDuration = aValues[2];

	if( bExtent )
		m_aExtendOpcodesTable[ iIndex ] = pInstruction;
	else
		m_aOpcodesTable[ iIndex ] = pInstruction;

	/*std::cout << pInstruction->m_sMnemonic << std::endl;
	std::cout << ( int )pInstruction->m_iLength << " " << (int)pInstruction->m_iDuration << " ";
	if ( pInstruction->m_iConditionalDuration != 0 )
		std::cout << (int)pInstruction->m_iConditionalDuration << std::endl;
	else
		std::cout << std::endl;

	if ( iFlags & Z )
	{
		if ( iFlagSet1 & Z)
			std::cout << "1 | ";
		else if ( iFlagReset0 & Z)
			std::cout << "0 | ";
		else
			std::cout << "Z | ";
	}
	else
		std::cout << "- | ";

	if ( iFlags & N )
	{
		if ( iFlagSet1 & N)
			std::cout << "1 | ";
		else if ( iFlagReset0 & N)
			std::cout << "0 | ";
		else
			std::cout << "N | ";
	}
	else
		std::cout << "- | ";

	if ( iFlags & H )
	{
		if ( iFlagSet1 & H)
			std::cout << "1 | ";
		else if ( iFlagReset0 & H)
			std::cout << "0 | ";
		else
			std::cout << "H | ";
	}
	else
		std::cout << "- | ";

	if ( iFlags & C )
	{
		if ( iFlagSet1 & C)
			std::cout << "1" << std::endl;
		else if ( iFlagReset0 & C)
			std::cout << "0" << std::endl;
		else
			std::cout << "C" << std::endl;
	}
	else
		std::cout << "-" << std::endl;
	std::cout << std::endl;*/
}

void Disassembler::Disassemble_ROM( const char* sRomPath )
{
	std::filesystem::path outputPath = DISASSM_DIR / static_cast<std::filesystem::path>( sRomPath ).filename();
	outputPath.replace_extension( ".asm" );

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
				oss << Format (" %02X", m_oCPU.GetDataAtAdress( iAdress + i ) );
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
				if ( m_oCPU.GetDataAtAdress( iAdress + i ) == 0 )
					break;
				oss << Format ("%c",m_oCPU.GetDataAtAdress( iAdress + i ) );
				++i;
			}
			break;
		}
		case 0x143:
		{
			switch ( m_oCPU.GetDataAtAdress( iAdress ) )
			{
				case 0:
					oss << Format (" %02X		;DMG - classic gameboy",m_oCPU.GetDataAtAdress( iAdress ) );
					break;
				case 0x80:
					oss << Format (" %02X		;CGB - retro compat monochrome",m_oCPU.GetDataAtAdress( iAdress ) );
					break;
				case 0xC0:
					oss << Format (" %02X		;CGB - only",m_oCPU.GetDataAtAdress( iAdress ) );
					break;
			}
			break;
		}
		case 0x147:
		{
			std::string sText;
			switch ( m_oCPU.GetDataAtAdress( iAdress ) )
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
			oss << Format (" %02X		;%s",m_oCPU.GetDataAtAdress( iAdress ), sText.c_str() );
			break;
		}
		case 0x148:
		{
			std::string sText;
			switch ( m_oCPU.GetDataAtAdress( iAdress ) )
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
			oss << Format (" %02X		;%s",m_oCPU.GetDataAtAdress( iAdress ), sText.c_str() );
			break;
		}
		case 0x149:
		{
			std::string sText;
			switch ( m_oCPU.GetDataAtAdress( iAdress ) )
			{
				case 0x00: sText =  "0, No RAM"; break;
				case 0x01: sText =  "-, Unused"; break;
				case 0x02: sText =  "8 KiB, 1 bank"; break;
				case 0x03: sText =  "32 KiB, 4 banks of 8 KiB each"; break;
				case 0x04: sText =  "128 KiB, 16 banks of 8 KiB each"; break;
				case 0x05: sText =  "64 KiB, 8 banks of 8 KiB each"; break;
				default:   sText =  "Unknown Code"; break;
			}
			oss << Format (" %02X		;%s",m_oCPU.GetDataAtAdress( iAdress ), sText.c_str() );
			break;
		}
		case 0x14A:
		{
			if ( m_oCPU.GetDataAtAdress( iAdress ) == 0 )
				oss << Format (" %02X		;Destination code : Japanese",m_oCPU.GetDataAtAdress( iAdress ) );
			else if ( m_oCPU.GetDataAtAdress( iAdress ) == 1 )
				oss << Format (" %02X		;Destination code : Overseas only",m_oCPU.GetDataAtAdress( iAdress ) );
			break;
		}
		case 0x14E:
		{
			oss << Format( " %02X %02X		",m_oCPU.GetDataAtAdress( iAdress ),m_oCPU.GetDataAtAdress( iAdress + 1 ) );
			break;
		}
		case 0x13F:
		case 0x144:
		case 0x146:
		case 0x14B:
		case 0x14C:
		case 0x14D:
		{
			oss << Format( " %02X	",m_oCPU.GetDataAtAdress( iAdress ) );
			break;
		}

		default:
			CPU_Instructions* pInstruction = m_aOpcodesTable[ m_oCPU.GetDataAtAdress( iAdress ) ];
			if( pInstruction != nullptr )
			{
				int i = 0;
				while( i < 4 )
				{
					if( iAdress + i < iAdress + pInstruction->m_iLength )
						oss << Format( " %02X",m_oCPU.GetDataAtAdress( iAdress + i ) );
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
				oss << Format( " %02X	",m_oCPU.GetDataAtAdress( iAdress ) );

			break;
	}


	oss << "	" << sComment;
	file << oss.str() << "\n";
}

void Disassembler::_FillOpcodesTables()
{
	for( uint16_t i = 0; i < 256; ++i )
	{
		uint8_t x = ( i & X_MASK ) >> 6;
		uint8_t y = ( i & Y_MASK ) >> 3;
		uint8_t p = ( i & P_MASK ) >> 4;
		uint8_t q = ( i & Q_MASK ) >> 3;
		uint8_t z = ( i & Z_MASK );

		switch( x )
		{
		case 0:
		{
			switch( z )
			{
			case 0:
			{
				switch( y )
				{
					case 0: AddCPUInstruction( i,{ &CPU::NOP }, 0,0,0,{1,4},false,"NOP" ); break;
					case 1: AddCPUInstruction( i,{ &CPU::LD },  0,0,0,{3,20},false,"LD (a16), SP" ); break;
					case 2: AddCPUInstruction( i,{ &CPU::STOP },0,0,0,{2,4},false,"STOP" ); break;
					case 3: AddCPUInstruction( i,{ &CPU::JR },  0,0,0,{2,12},false,"JR s8" ); break;
					case 4:
					case 5:
					case 6:
					case 7:
						AddCPUInstruction( i,{ &CPU::JR },0,0,0,{2,12,8},false,"JR %s s8",cc[ y-4 ].data() ); break;
				}
				break;
			}
			case 1:
			{
				switch( q )
				{
					case 0: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{3,12},false,"LD %s, d16",rp[ p ].data() ); break;
					case 1: AddCPUInstruction( i,{ &CPU::ADD },static_cast< uint8_t >( N | H | C ), (0), (N),{1,8},false,"ADD HL, %s",rp[ p ].data() ); break;
				}
				break;
			}
			case 2:
			{
				switch( q )
				{
				case 0:
				{
					switch( p )
					{
						case 0: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD (BC), A" ); break;
						case 1: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD (DE), A" ); break;
						case 2: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD (HL+), A" ); break;
						case 3: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD (HL-), A" ); break;
					}
					break;
				}
				case 1:
				{
					switch( p )
					{
						case 0: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD A, (BC)" ); break;
						case 1: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD A, (DE)" ); break;
						case 2: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD A, (HL+)" ); break;
						case 3: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD A, (HL-)" ); break;
					}
					break;
				}
				}
				break;
			}
			case 3:
			{
				switch( q )
				{
				case 0: AddCPUInstruction( i,{ &CPU::INC },0,0,0,{1,8},false,"INC, %s",rp[ p ].data() ); break;
				case 1: AddCPUInstruction( i,{ &CPU::DEC },0,0,0,{1,8},false,"DEC, %s",rp[ p ].data() ); break;
				}
				break;
			}
			case 4:
			{
				if( y == 6 )
					AddCPUInstruction( i,{ &CPU::INC },( Z | N | H ),0, ( N ),{1,12},false,"INC (HL)" );
				else
					AddCPUInstruction( i,{ &CPU::INC },( Z | N | H ),0, ( N ),{1,4},false,"INC, %c",r[ y ] );
				break;
			}
			case 5:
			{
				if( y == 6 )
					AddCPUInstruction( i,{ &CPU::DEC },(Z | N | H),( N ), 0,{1,12},false,"DEC (HL)" );
				else
					AddCPUInstruction( i,{ &CPU::DEC },(Z | N | H),( N ), 0,{1,4},false,"DEC, %c",r[ y ] );
				break;
			}
			case 6:
			{
				if( y == 6 )
					AddCPUInstruction( i,{ &CPU::LD },0,0,0,{2,12},false,"LD (HL), d8" );
				else
					AddCPUInstruction( i,{ &CPU::LD },0,0,0,{2,8},false,"LD, %c, d8",r[ y ] );
				break;
			}
			case 7:
			{
				switch( y )
				{
					case 0: AddCPUInstruction( i,{ &CPU::RLCA },( Z | N | H | C ),  0,		 ( Z | N | H ), {1,4},false,"RLCA" ); break;
					case 1: AddCPUInstruction( i,{ &CPU::RRCA },( Z | N | H | C ),  0,		 ( Z | N | H ), {1,4},false,"RRCA" ); break;
					case 2: AddCPUInstruction( i,{ &CPU::RLA }, ( Z | N | H | C ),  0,		 ( Z | N | H ), {1,4},false,"RLA" ); break;
					case 3: AddCPUInstruction( i,{ &CPU::RRA }, ( Z | N | H | C ),  0,		 ( Z | N | H ), {1,4},false,"RRA" ); break;
					case 4: AddCPUInstruction( i,{ &CPU::DAA }, ( Z     | H | C ),  0,		 (		   H ),	{1,4},false,"DAA" ); break;
					case 5: AddCPUInstruction( i,{ &CPU::CPL }, (	   N | H	), ( N | H	),		   0,	{1,4},false,"CPL" ); break;
					case 6: AddCPUInstruction( i,{ &CPU::SCF }, (     N | H | C ),( C ),	 (	   N | H ),	{1,4},false,"SCF" ); break;
					case 7: AddCPUInstruction( i,{ &CPU::CCF }, (     N | H | C ),  0,		 (     N | H ),	{1,4},false,"CCF" ); break;
				}
				break;
			}
			}
			break;
		}
		case 1:
		{
			if( z == 6 && y == 6 )
			{
				AddCPUInstruction( i,{ &CPU::HALT },0,0,0,{1,4},false,"HALT" );
			}
			else
			{
				if( y == 6 )
					AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD (HL), %c",r[ z ] );
				else if( z == 6 )
					AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD %c, (HL)",r[ y ] );
				else
					AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,4},false,"LD %c, %c",r[ y ],r[ z ] );
			}
			break;
		}
		case 2:
		{
			switch( y )
			{
				case 0:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &CPU::ADD },( Z | N | H | C ),0,( N ),{1,8},false,"%s (HL)", alu[ y ].data(),r[ z ] );
					else
						AddCPUInstruction( i,{ &CPU::ADD },( Z | N | H | C ),0,( N ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 1:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &CPU::ADC },( Z | N | H | C ),0,( N ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
						AddCPUInstruction( i,{ &CPU::ADC },( Z | N | H | C ),0,( N ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 2:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &CPU::SUB },( Z | N | H | C ),( N ), 0,{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							AddCPUInstruction( i,{ &CPU::SUB },( Z | N | H | C ),( Z | N ),( H | C ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
						else
							AddCPUInstruction( i,{ &CPU::SUB },( Z | N | H | C ),(		N ),	0	 ,{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 3:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &CPU::SBC },	( Z | N | H | C ),( N ), 0,	{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							AddCPUInstruction( i,{ &CPU::SBC },( Z | N | H ),	  ( N ), 0,	{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							AddCPUInstruction( i,{ &CPU::SBC },( Z | N | H | C ),( N ), 0,	{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 4:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &CPU::AND },( Z | N | H | C ), ( H ),( N | C ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
						AddCPUInstruction( i,{ &CPU::AND },( Z | N | H | C ), ( H ),( N | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 5:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &CPU::XOR },	( Z | N | H | C ), ( Z ),( N | H | C ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							AddCPUInstruction( i,{ &CPU::XOR },( Z | N | H | C ), ( Z ),( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							AddCPUInstruction( i,{ &CPU::XOR },( Z | N | H | C ),	 0,	 ( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 6:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &CPU::OR },( Z | N | H | C ),	0,	( N | H | C ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
						AddCPUInstruction( i,{ &CPU::OR },( Z | N | H | C ), ( Z ),( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 7:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &CPU::CP },		( Z | N | H | C ),		0,	( N ),			{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							AddCPUInstruction( i,{ &CPU::CP }, ( Z | N | H | C ), ( Z | N ),	( H | C ),	{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							AddCPUInstruction( i,{ &CPU::CP }, ( Z | N | H | C ), (	 N ),	0,			{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}
			}
			break;
		}
		case 3:
		{
			switch( z )
			{
			case 0:
			{
				switch( y )
				{
					case 0:
					case 1:
					case 2:
					case 3:
							AddCPUInstruction( i,{ &CPU::RET },	0,			  0,	0,		{1,20,8},false,"RET %s",cc[ y ].data() ); break;
					case 4: AddCPUInstruction( i,{ &CPU::LD },		0,			  0,	0,		{2,12},false,"LD (a8), A" ); break;
					case 5: AddCPUInstruction( i,{ &CPU::ADD },( Z | N | H | C ),0,( Z | N ),  {2,16},false,"ADD SP, s8" ); break;
					case 6: AddCPUInstruction( i,{ &CPU::LD },		0,			  0,	0,		{2,8},false,"LD A, (a8)" ); break;
					case 7: AddCPUInstruction( i,{ &CPU::LD }, ( Z | N | H | C ),0,( Z | N ),  {2,12},false,"LD HL, SP+s8" ); break;
				}
				break;
			}
			case 1:
			{
				switch( q )
				{
				case 0:
				{
					if( p == 3 ) //AF
						AddCPUInstruction( i,{ &CPU::POP },( Z | N | H | C ),0,0,{1,12},false,"POP %s",rp2[ p ].data() );
					else
						AddCPUInstruction( i,{ &CPU::POP },	  0,		  0,0,{1,12},false,"POP %s",rp2[ p ].data() );
					break;
				}
				case 1:
				{
					switch( p )
					{
						case 0: AddCPUInstruction( i,{ &CPU::RET }, 0,0,0,{1,16},false,"RET" ); break;
						case 1: AddCPUInstruction( i,{ &CPU::RETI },0,0,0,{1,16},false,"RETI" ); break;
						case 2: AddCPUInstruction( i,{ &CPU::JP },  0,0,0,{1,4},false,"JP HL" ); break;
						case 3: AddCPUInstruction( i,{ &CPU::LD },  0,0,0,{1,8},false,"LD SP, HL" ); break;
					}
					break;
				}
				}
				break;
			}
			case 2:
			{
				switch( y )
				{
					case 0:
					case 1:
					case 2:
					case 3:
							AddCPUInstruction( i,{ &CPU::JP },0,0,0,{3,16,12},false,"JP %s a16",cc[ y ].data() ); break;
					case 4: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD (C), A",cc[ y ].data() ); break;
					case 5: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{3,16},false,"LD a16, A" ); break;
					case 6: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD A, (C)" ); break;
					case 7: AddCPUInstruction( i,{ &CPU::LD },0,0,0,{3,16},false,"JP A, a16" ); break;
				}
				break;
			}
			case 3:
			{
				switch( y )
				{
				case 0: AddCPUInstruction( i,{ &CPU::JP },0,0,0,{3,16},false,"JP a16" ); break;
				case 1:
				{
					for( int k = 0; k < 256; ++k )
					{
						x = ( k & X_MASK ) >> 6;
						y = ( k & Y_MASK ) >> 3;
						z = ( k & Z_MASK );

						switch( x )
						{
						case 0:
						{
							const char* format = "%s %c";
							uint8_t iDuration = 8;
							if ( z == 6 )
							{
								format =  "%s (HL)";
								iDuration = 16;
							}
							switch( y )
							{
								case 0: AddCPUInstruction( k,{ &CPU::RLC }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 1: AddCPUInstruction( k,{ &CPU::RRC }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 2: AddCPUInstruction( k,{ &CPU::RL },  ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 3: AddCPUInstruction( k,{ &CPU::RR },  ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 4: AddCPUInstruction( k,{ &CPU::SLA }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 5: AddCPUInstruction( k,{ &CPU::SRA }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 6: AddCPUInstruction( k,{ &CPU::SWAP },( Z | N | H | C ), 0,( N | H | C ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 7: AddCPUInstruction( k,{ &CPU::SRL }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
							}
							break;
						}
						case 1:
						{
							if( z == 6 )
								AddCPUInstruction( k,{ &CPU::BIT },( Z | N | H ), ( H ),( N ),{2,12},true,"BIT %i, (HL)",y );
							else
								AddCPUInstruction( k,{ &CPU::BIT },( Z | N | H ), ( H ),( N ),{2,8},true,"BIT %i, %c",y,r[ z ] );
							break;
						}
						case 2:
						{
							if( z == 6 )
								AddCPUInstruction( k,{ &CPU::RES },0,0,0,{2,16},true,"RES %i, (HL)",y );
							else
								AddCPUInstruction( k,{ &CPU::RES },0,0,0,{2,8},true,"RES %i, %c",y,r[ z ] );
							break;
						}
						case 3:
						{
							if( z == 6 )
								AddCPUInstruction( k,{ &CPU::SET },0,0,0,{2,16},true,"SET %i, (HL)",y );
							else
								AddCPUInstruction( k,{ &CPU::SET },0,0,0,{2,8},true,"SET %i, %c",y,r[ z ] );
						}
						}
					}
					break;
				}
				case 6: AddCPUInstruction( i,{ &CPU::DI },0,0,0,{1,4},false,"DI" ); break;
				case 7: AddCPUInstruction( i,{ &CPU::EI },0,0,0,{1,4},false,"EI" ); break;
				}
				break;
			}
			case 4:
			{
				switch( y )
				{
					case 0:
					case 1:
					case 2:
					case 3:
						AddCPUInstruction( i,{ &CPU::PUSH },0,0,0,{3,24,12},false,"CALL %s, a16",cc[ y ].data() ); break;
				}
				break;
			}
			case 5:
			{
				if( q == 0 )
					AddCPUInstruction( i,{ &CPU::PUSH },0,0,0,{1,16},false,"PUSH %s",rp2[ p ].data() );
				else if( q == 1 && p == 0 )
					AddCPUInstruction( i,{ &CPU::CALL },0,0,0,{3,24},false,"CALL a16" );
				break;
			}
			case 6:
			{
				switch( y )
				{
					case 0: AddCPUInstruction( i,{ &CPU::ADD },( Z | N | H | C ),   0,	  ( N ),		{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 1: AddCPUInstruction( i,{ &CPU::ADC },( Z | N | H | C ),   0,	  ( N ),		{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 2: AddCPUInstruction( i,{ &CPU::SUB },( Z | N | H | C ), ( N ),   0,			{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 3: AddCPUInstruction( i,{ &CPU::SBC },( Z | N | H | C ), ( N ),   0,			{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 4: AddCPUInstruction( i,{ &CPU::AND },( Z | N | H | C ), ( H ), ( N | C ),	{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 5: AddCPUInstruction( i,{ &CPU::XOR },( Z | N | H | C ),   0,	  ( N | H | C ),{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 6: AddCPUInstruction( i,{ &CPU::OR }, ( Z | N | H | C ),   0,	  ( N | H | C ),{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 7: AddCPUInstruction( i,{ &CPU::CP }, ( Z | N | H | C ), ( N ),   0,			{2,8},false,"%s d8",alu[ y ].data() ); break;
				}
				break;
			}
			case 7: AddCPUInstruction( i,{ &CPU::RST },0,0,0,{1,16},false,"RST %i",y ); break;
			}
			break;
		}
		default: std::cout << "Case not handled" << std::endl; break;
		}
	}
}
