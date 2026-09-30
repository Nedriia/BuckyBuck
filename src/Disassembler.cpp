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

#include <nlohmann/json.hpp>

using json = nlohmann::json;

static int g_iCounter = 0;
CPU* Disassembler::m_pCPU = nullptr;
std::vector< Disassembler::DisassembledLine > Disassembler::m_aDisassembly;

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
	std::filesystem::path outputPath = DISASSM_DIR / static_cast<std::filesystem::path>( sRomPath ).filename();//don't use name but calculate HASH
	outputPath.replace_extension( ".json" );

	if ( exists( outputPath ) == false )
	{
		std::fstream file;
		file.open( outputPath.string(), std::ofstream::out );

		if( file.is_open() )
		{
			std::string sComment = "";
			json data;
			data["Instructions"] = json::array();

			g_iCounter = 0;
			uint8_t iLengthIncrease = 0;

			for ( uint16_t iPC = 0x000; iPC < 0x7FFF; )//Size ROM
			{
				if ( iPC >= 0x104 && iPC <= 0x14F )
				{
					DecryptCartridge( file, data );
					iPC = 0x150;
					continue;
				}

				_WriteInstruction( data, iPC, sComment,&iLengthIncrease );
				iPC += iLengthIncrease;
			}
			file << data.dump(4);
		}
	}

	//Read the file to feed to the disassembler display
	std::ifstream file( outputPath );
	if( file.is_open() )
	{
		json data;
		try
		{
			data = json::parse(file);
		}
		catch (const json::parse_error& e)
		{
			std::cerr << e.what() << '\n';
			return;
		}

		for ( const auto& instruction : data["Instructions"])
		{
			DisassembledLine oDisasLine;
			oDisasLine.m_iAdress = instruction.value("address", "");
			oDisasLine.m_sMnemonic = instruction.value("text", "");
			oDisasLine.m_sAditionalInfo = instruction.value("Comment", "");
			oDisasLine.m_oData = instruction.value("bytes", "");
			oDisasLine.m_iDuration = instruction.value("duration", 0xFF );

			m_aDisassembly.push_back( oDisasLine );
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

void Disassembler::DecryptCartridge( std::fstream& file, json& oData )
{
	std::string sComment = ";Nintendo Logo";
	_WriteInstruction( oData, 0x104,sComment  );

	sComment = ";Title";
	_WriteInstruction( oData, 0x134, sComment );

	sComment = ";Manufacturer code";
	_WriteInstruction( oData, 0x13F, sComment );

	sComment = ";CGB Flag";
	_WriteInstruction( oData, 0x143, sComment );

	sComment = ";New Licence Code";
	_WriteInstruction( oData, 0x144, sComment );

	sComment = ";SGB Flag";
	_WriteInstruction( oData, 0x146, sComment );

	sComment = ";Cartridge Type";
	_WriteInstruction( oData, 0x147, sComment );

	sComment = ";ROM size";
	_WriteInstruction( oData, 0x148, sComment );

	sComment = ";RAM size";
	_WriteInstruction( oData, 0x149, sComment );

	sComment = ";Destination code";
	_WriteInstruction( oData, 0x14A, sComment );

	sComment = ";Old Licence code";
	_WriteInstruction( oData, 0x14B, sComment );

	sComment = ";Mask ROM version number";
	_WriteInstruction( oData, 0x14C, sComment );

	sComment = ";Header Checksum";
	_WriteInstruction( oData, 0x14D, sComment );

	sComment = ";Global Checksum";
	_WriteInstruction( oData, 0x14E, sComment );
}

void Disassembler::DecryptIORange( std::fstream& file )
{
	// _WriteInstruction( file, 0xFF00, ";Joypad");
	// _WriteInstruction( file, 0xFF01, ";Serial data");
	// _WriteInstruction( file, 0xFF02, ";Serial control");
	// _WriteInstruction( file, 0xFF03, "");
	// _WriteInstruction( file, 0xFF04, ";Divider");
	// _WriteInstruction( file, 0xFF05, ";Timer counter");
	// _WriteInstruction( file, 0xFF06, ";Timer modulo");
	// _WriteInstruction( file, 0xFF07, ";Timer ctrl");
	// _WriteInstruction( file, 0xFF08, "");
	// _WriteInstruction( file, 0xFF0F, ";Int flag");
	// for ( int i = 0xFF10; i <= 0xFF26; ++i )
	// 	_WriteInstruction( file, i, Format( "NR%i", i ).c_str());
	// _WriteInstruction( file, 0xFF27, "");
	// _WriteInstruction( file, 0xFF30, ";Wave pattern");
	// _WriteInstruction( file, 0xFF40, ";lcd ctrl");
	// _WriteInstruction( file, 0xFF41, ";lcd stat");
	// _WriteInstruction( file, 0xFF42, ";scroll Y");
	// _WriteInstruction( file, 0xFF43, ";scroll X");
	// _WriteInstruction( file, 0xFF44, ";LY");
	// _WriteInstruction( file, 0xFF45, ";LYC");
	// _WriteInstruction( file, 0xFF46, ";OAM DMA");
	// _WriteInstruction( file, 0xFF47, ";bg pal");
	// _WriteInstruction( file, 0xFF48, ";obj pal 0");
	// _WriteInstruction( file, 0xFF49, ";obj pal 1");
	// _WriteInstruction( file, 0xFF4A, ";win Y");
	// _WriteInstruction( file, 0xFF4B, ";win X");
	// _WriteInstruction( file, 0xFF4D, ";speed switch");
	// _WriteInstruction( file, 0xFF4F, ";vram bank");
	// _WriteInstruction( file, 0xFF50, ";disable bootrom");
	// _WriteInstruction( file, 0xFF51, ";hdma src hi");
	// _WriteInstruction( file, 0xFF52, ";hdma src low");
	// _WriteInstruction( file, 0xFF53, ";hdma dest hi");
	// _WriteInstruction( file, 0xFF54, ";hdma dest low");
	// _WriteInstruction( file, 0xFF55, ";hdma count");
	// _WriteInstruction( file, 0xFF56, ";ir port");
	// _WriteInstruction( file, 0xFF68, ";bg pal set");
	// _WriteInstruction( file, 0xFF69, ";bg pal data");
	// _WriteInstruction( file, 0xFF6A, ";obj pal sel");
	// _WriteInstruction( file, 0xFF6B, ";obj pal data");
	// _WriteInstruction( file, 0xFF70, ";wram bank");
	// _WriteInstruction( file, 0xFF76, ";PCM34");
	// _WriteInstruction( file, 0xFF77, ";PCM12");
}

void Disassembler::_WriteInstruction( json& oData, uint16_t iAdress, std::string& sComment, uint8_t* iLengthIncrease /*= nullptr*/ )
{
	json block = json::object();
	std::stringstream ss;
	std::stringstream sAdress;
	sAdress << std::hex
			<< std::uppercase
			<< "0X"
			<< std::setw(8)
			<< std::setfill('0')
			<< static_cast<int>( iAdress );

	switch ( iAdress )
	{
		case 0x104 :
		{
			int i = 0;
			while ( i + iAdress < 0x134 )
			{
				if ( i != 0 && i % 16 == 0 )
				{
					block["address"] = sAdress.str();
					block["bytes"]   = ss.str();

					oData["Instructions"].push_back( block );

					block = json::object();
					sAdress.str("");
					sAdress.clear();
					sAdress << std::hex
							<< std::uppercase
							<< "0X"
							<< std::setw(8)
							<< std::setfill('0')
							<< static_cast<int>( iAdress + i );

					ss.str("");
					ss.clear();
				}

				ss	<< std::hex
					<< std::uppercase
					<< std::setw(2)
					<< std::setfill('0')
					<< static_cast<int>(m_pCPU->GetDataAtAdress( iAdress + i ) )
					<< " ";

				++i;
			}

			block["address"]	= sAdress.str();
			block["bytes"]		= ss.str();
			block["Comment"]    = sComment;

			break;
		}
		case 0x134:
		{
			std::stringstream sTitle;
			while ( iAdress < 0x143 )
			{
				int iData =  static_cast<int> ( m_pCPU->GetDataAtAdress( iAdress ) );
				if ( iData != 0 )
				{
					ss << std::hex
						<< std::uppercase
						<< std::setw(2)
						<< std::setfill('0')
						<< iData
						<< " ";

					sTitle << static_cast<char>( iData );
					++iAdress;
				}
				else
					break;
			}

			block["address"]	= sAdress.str();
			block["bytes"]		= ss.str();
			block["text"]		= sTitle.str();
			block["Comment"]    = sComment;
			break;
		}
		case 0x143:
		{
			std::string sText;
			ss << std::hex
				<< std::uppercase
				<< std::setw(2)
				<< std::setfill('0')
				<< static_cast<int>(m_pCPU->GetDataAtAdress( iAdress ) );

			switch ( m_pCPU->GetDataAtAdress( iAdress ) )
			{
				case 0:		sComment += " ;DMG - classic gameboy"; break;
				case 0x80:	sComment += " ;CGB - retro compat monochrome"; break;
				case 0xC0:	sComment += " ;CGB - only"; break;
			}

			block["address"]	= sAdress.str();
			block["bytes"]		= ss.str();
			block["Comment"]    = sComment;
			block["text"]		= sText;
			break;
		}
		case 0x147:
		{
			std::string sText;
			uint8_t iData = m_pCPU->GetDataAtAdress( iAdress );
			switch ( iData )
			{
				case 0x00: sComment += " ROM ONLY" ; break;
				case 0x01: sComment += " MBC1" ; break;
				case 0x02: sComment += " MBC1+RAM" ; break;
				case 0x03: sComment += " MBC1+RAM+BATTERY" ; break;
				case 0x05: sComment += " MBC2" ; break;
				case 0x06: sComment += " MBC2+BATTERY" ; break;
				case 0x08: sComment += " ROM+RAM" ; break;
				case 0x09: sComment += " ROM+RAM+BATTERY" ; break;
				case 0x0B: sComment += " MMM01" ; break;
				case 0x0C: sComment += " MMM01+RAM" ; break;
				case 0x0D: sComment += " MMM01+RAM+BATTERY" ; break;
				case 0x0F: sComment += " MBC3+TIMER+BATTERY" ; break;
				case 0x10: sComment += " MBC3+TIMER+RAM+BATTERY" ; break;
				case 0x11: sComment += " MBC3" ; break;
				case 0x12: sComment += " MBC3+RAM" ; break;
				case 0x13: sComment += " MBC3+RAM+BATTERY" ; break;
				case 0x19: sComment += " MBC5" ; break;
				case 0x1A: sComment += " MBC5+RAM" ; break;
				case 0x1B: sComment += " MBC5+RAM+BATTERY" ; break;
				case 0x1C: sComment += " MBC5+RUMBLE" ; break;
				case 0x1D: sComment += " MBC5+RUMBLE+RAM" ; break;
				case 0x1E: sComment += " MBC5+RUMBLE+RAM+BATTERY" ; break;
				case 0x20: sComment += " MBC6" ; break;
				case 0x22: sComment += " MBC7+SENSOR+RUMBLE+RAM+BATTERY" ; break;
				case 0xFC: sComment += " POCKET CAMERA" ; break;
				case 0xFD: sComment += " BANDAI TAMA5" ; break;
				case 0xFE: sComment += " HuC3" ; break;
				case 0xFF: sComment += " HuC1+RAM+BATTERY" ; break;
				default:   sComment += " Unknown Type" ; break;
			}

			ss << std::hex
				<< std::uppercase
				<< std::setw(2)
				<< std::setfill('0')
				<< static_cast<int>( iData );

			block["address"]	= sAdress.str();
			block["bytes"]		= ss.str();
			block["Comment"]    = sComment;
			block["text"]		= sText;
			break;
		}
		case 0x148:
		{
			std::string sText;
			uint8_t iData = m_pCPU->GetDataAtAdress( iAdress );
			switch ( iData )
			{
				case 0x00: sComment += " 32 KiB, 2 (no banking)"; break;
				case 0x01: sComment += " 64 KiB, 4"; break;
				case 0x02: sComment += " 128 KiB, 8"; break;
				case 0x03: sComment += " 256 KiB, 16"; break;
				case 0x04: sComment += " 512 KiB, 32"; break;
				case 0x05: sComment += " 1 MiB, 64"; break;
				case 0x06: sComment += " 2 MiB, 128"; break;
				case 0x07: sComment += " 4 MiB, 256"; break;
				case 0x08: sComment += " 8 MiB, 512"; break;
				case 0x52: sComment += " 1.1 MiB, 72"; break;
				case 0x53: sComment += " 1.2 MiB, 80"; break;
				case 0x54: sComment += " 1.5 MiB, 96"; break;
				default:   sComment += " Unknown Value"; break;
			}

			ss << std::hex
				<< std::uppercase
				<< std::setw(2)
				<< std::setfill('0')
				<< static_cast<int>( iData );

			block["address"] = sAdress.str();
			block["bytes"]   = ss.str();
			block["Comment"]    = sComment;
			block["text"]    = sText;
			break;
		}
		case 0x149:
		{
			std::string sText;
			uint8_t iData = m_pCPU->GetDataAtAdress( iAdress );

			switch ( iData )
			{
				case 0x00: sComment += " 0, No RAM"; break;
				case 0x01: sComment += " -, Unused"; break;
				case 0x02: sComment += " 8 KiB, 1 bank"; break;
				case 0x03: sComment += " 32 KiB, 4 banks of 8 KiB each"; break;
				case 0x04: sComment += " 128 KiB, 16 banks of 8 KiB each"; break;
				case 0x05: sComment += " 64 KiB, 8 banks of 8 KiB each"; break;
				default:   sComment += " Unknown Code"; break;
			}

			ss << std::hex
				<< std::uppercase
				<< std::setw(2)
				<< std::setfill('0')
				<< static_cast<int>( iData );

			block["address"] = sAdress.str();
			block["bytes"]   = ss.str();
			block["Comment"]    = sComment;
			block["text"]    = sText;
			break;
		}
		case 0x14A:
		{
			ss << std::hex
				<< std::uppercase
				<< std::setw(2)
				<< std::setfill('0')
				<< static_cast<int>( m_pCPU->GetDataAtAdress( iAdress ) );

			block["address"]	= sAdress.str();
			block["bytes"]		= ss.str();
			if ( m_pCPU->GetDataAtAdress( iAdress ) == 0 )
				sComment    += " : Japanese" ;
			else if ( m_pCPU->GetDataAtAdress( iAdress ) == 1 )
				sComment    += " : Overseas only" ;
			block["Comment"]    = sComment;
			break;
		}
		case 0x14E:
		{
			ss << std::hex
				<< std::uppercase
				<< std::setw(2)
				<< std::setfill('0')
				<< static_cast<int>( m_pCPU->GetDataAtAdress( iAdress ) )
				<< " "
				<< static_cast<int>( m_pCPU->GetDataAtAdress( iAdress + 1 ) );

			block["address"] = sAdress.str();
			block["bytes"]   = ss.str();
			block["Comment"]    = sComment;
			break;
		}
		case 0x13F:
		case 0x144:
		case 0x146:
		case 0x14B:
		case 0x14C:
		case 0x14D:
		{
			ss << std::hex
				<< std::uppercase
				<< std::setw(2)
				<< std::setfill('0')
				<< static_cast<int>( m_pCPU->GetDataAtAdress( iAdress ) )
				<< " ";

			block["address"]	= sAdress.str();
			block["bytes"]		= ss.str();
			block["Comment"]    = sComment;
			break;
		}

		default:
		{
			if (  m_pCPU->GetDataAtAdress( iAdress ) == 0 )
			{
				block["address"]	= sAdress.str();
				block["bytes"]		= "00";
				block["text"]		= "NOP";
				block["duration"]		= 1;

				if( iLengthIncrease )
					*iLengthIncrease = 1;
			}
			else
			{
				uint8_t iIndex = m_pCPU->GetDataAtAdress( iAdress );
				CPU::CPU_Instructions* pInstruction = iIndex == 0xCB ? CPU::m_aExtendOpcodesTable[ m_pCPU->GetDataAtAdress( iAdress + 1 ) ] : CPU::m_aOpcodesTable[ iIndex ]  ;
				if( pInstruction != nullptr )
				{
					int i = 0;
					while( i < pInstruction->m_iLength )
					{
						ss << std::hex
						   << std::uppercase
						   << std::setw(2)
						   << std::setfill('0')
						   << static_cast<int>(m_pCPU->GetDataAtAdress( iAdress + i) )
						   << " ";
						++i;
					}

					g_iCounter += pInstruction->m_iDuration >> 2;
					if( iLengthIncrease )
						*iLengthIncrease = pInstruction->m_iLength;

					block["address"] = sAdress.str();
					block["bytes"]   = ss.str();
					block["text"]    = pInstruction->m_sMnemonic;
					block["duration"]  = pInstruction->m_iDuration / 4; //TEMP -> t cycle to m cycle
				}
				else
				{
					block["address"] = sAdress.str();
					ss << std::hex
					   << std::uppercase
					   << std::setw(2)
					   << std::setfill('0')
					   << static_cast<int>( m_pCPU->GetDataAtAdress( iAdress ) );
					block["bytes"]		= ss.str();
					block["text"]		= "undefined opcode";
					block["duration"]		= 0;
				}
				break;
			}
		}
	}
	oData["Instructions"].push_back( block );
}