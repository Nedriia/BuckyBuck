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

using json = nlohmann::json;

static int g_iCounter = 0;
CPU* Disassembler::m_pCPU = nullptr;
std::vector< Disassembler::DisassembledLine > Disassembler::m_aDisassembly;
uint8_t SAXParser::m_iElementFlag = 0;
uint8_t SAXParser::m_iDepth = 0;

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
	auto start = std::chrono::high_resolution_clock::now();

	if( sRomPath == nullptr )
		return;

	std::filesystem::path outputPath = DISASSM_DIR / static_cast< std::filesystem::path >( sRomPath ).filename();//don't use name but calculate HASH
	outputPath.replace_extension( ".json" );

	//Disassemble instructions
	if( exists( outputPath ) == false )
	{
		std::fstream file;
		file.open( outputPath.string(),std::ofstream::out | std::ios::binary );

		if( file.is_open() )
		{
			CBORWriter oCborWriter;
			oCborWriter.buffer.reserve( 1 << 20 );
			oCborWriter.Map( 1 );
			oCborWriter.Text( "Instructions", 12 );
			oCborWriter.Array_Begin();
			oCborWriter.SizePlaceHolder();

			g_iCounter = 0;
			for( uint32_t iPC = 0; iPC < m_pCPU->GetMemorySize(); )
			{
				if( iPC >= 0x104 && iPC <= 0x14F )
				{
					DecryptCartridge( oCborWriter );
					iPC = 0x150;
					continue;
				}

				iPC += _WriteInstruction( oCborWriter,iPC );
			}

			oCborWriter.Break();
			file.write( reinterpret_cast< const char* >( oCborWriter.buffer.data() ), oCborWriter.buffer.size() );
		}
	}
	//TODO : tmp file
	auto end = std::chrono::high_resolution_clock::now();
	auto iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
	start = end;
	std::cout << "Disassemble Time " << iDurationMs;

	////Read the file to feed to the disassembler visual display
	std::ifstream file( outputPath,std::ios::binary | std::ios::ate );
	if( file.is_open() )
	{
		const std::streamsize size = file.tellg();
		file.seekg( 0 );
		std::vector<uint8_t> buffer( static_cast< size_t >( size ) );
		file.read( reinterpret_cast< char* >( buffer.data() ),size );

		m_aDisassembly.clear();
		SAXParser oSaxParser;
		bool bResult = json::sax_parse( buffer, &oSaxParser, nlohmann::detail::input_format_t::cbor, true, true );
		if ( bResult == false )
			m_aDisassembly.clear();
	}

	end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
	std::cout << " + Reading Time " << iDurationMs << std::endl;
}

std::string Disassembler::Format( const char* sFormat,... )
{
	va_list args;
	va_start( args,sFormat );
	size_t len = std::vsnprintf( nullptr,0,sFormat,args );
	va_end( args );

	std::vector<char> vec( len + 1 );
	va_start( args,sFormat );
	std::vsnprintf( &vec[ 0 ],len + 1,sFormat,args );
	va_end( args );

	return &vec[ 0 ];
}

void Disassembler::DecryptCartridge( CBORWriter& oData )
{
	_WriteInstruction( oData,0x104 );
	_WriteInstruction( oData,0x134 );
	_WriteInstruction( oData,0x13F );
	_WriteInstruction( oData,0x143 );
	_WriteInstruction( oData,0x144 );
	_WriteInstruction( oData,0x146 );
	_WriteInstruction( oData,0x147 );
	_WriteInstruction( oData,0x148 );
	_WriteInstruction( oData,0x149 );
	_WriteInstruction( oData,0x14A );
	_WriteInstruction( oData,0x14B );
	_WriteInstruction( oData,0x14C );
	_WriteInstruction( oData,0x14D );
	_WriteInstruction( oData,0x14E );
}

void Disassembler::DecryptIORange( CBORWriter& oData )
{
	_WriteInstruction( oData, 0xFF00 );
	_WriteInstruction( oData, 0xFF01 );
	_WriteInstruction( oData, 0xFF02 );
	_WriteInstruction( oData, 0xFF03 );
	_WriteInstruction( oData, 0xFF04 );
	_WriteInstruction( oData, 0xFF05 );
	_WriteInstruction( oData, 0xFF06 );
	_WriteInstruction( oData, 0xFF07 );
	_WriteInstruction( oData, 0xFF08 );
	_WriteInstruction( oData, 0xFF0F );
	for ( int i = 0xFF10; i <= 0xFF26; ++i )
		_WriteInstruction( oData, i );
	_WriteInstruction( oData, 0xFF27 );
	_WriteInstruction( oData, 0xFF30 );
	_WriteInstruction( oData, 0xFF40 );
	_WriteInstruction( oData, 0xFF41 );
	_WriteInstruction( oData, 0xFF42 );
	_WriteInstruction( oData, 0xFF43 );
	_WriteInstruction( oData, 0xFF44 );
	_WriteInstruction( oData, 0xFF45 );
	_WriteInstruction( oData, 0xFF46 );
	_WriteInstruction( oData, 0xFF47 );
	_WriteInstruction( oData, 0xFF48 );
	_WriteInstruction( oData, 0xFF49 );
	_WriteInstruction( oData, 0xFF4A );
	_WriteInstruction( oData, 0xFF4B );
	_WriteInstruction( oData, 0xFF4D );
	_WriteInstruction( oData, 0xFF4F );
	_WriteInstruction( oData, 0xFF50 );
	_WriteInstruction( oData, 0xFF51 );
	_WriteInstruction( oData, 0xFF52 );
	_WriteInstruction( oData, 0xFF53 );
	_WriteInstruction( oData, 0xFF54 );
	_WriteInstruction( oData, 0xFF55 );
	_WriteInstruction( oData, 0xFF56 );
	_WriteInstruction( oData, 0xFF68 );
	_WriteInstruction( oData, 0xFF69 );
	_WriteInstruction( oData, 0xFF6A );
	_WriteInstruction( oData, 0xFF6B );
	_WriteInstruction( oData, 0xFF70 );
	_WriteInstruction( oData, 0xFF76 );
	_WriteInstruction( oData, 0xFF77 );
}

uint8_t Disassembler::_WriteInstruction( CBORWriter& oData,uint16_t _iAdress )
{
	uint8_t					iDuration = 0;
	uint16_t				iAdress = static_cast< int >( _iAdress );
	std::string				sMnemonic;
	char					aData[ 64 ] = { '\0' };
	std::string				sComment = "";
	uint8_t					iPCIncrease = 1;
	uint8_t					iLengthData = 1;

	if( iAdress >= 0xFF10 && iAdress <= 0xFF26 )
		sComment = Format( "NR%i", iAdress - 0xFF10 ).c_str();
	else
	{
		switch( iAdress )
		{
			case 0x104:
			{
				sComment = ";Nintendo Logo";
				for( int i = 0; i < 6; ++i )
				{
					ToHex( iAdress + 8 * i,aData,8 );
					WriteBlock( oData,iAdress + 8 * i,sMnemonic.c_str(),i == 5 ? sComment.c_str() : "",aData,0);
				}
				iPCIncrease = 48;
				return iPCIncrease;
			}
			case 0x134:
			{
				sComment = ";Title";
				iLengthData = 15;
				sMnemonic.resize( iLengthData );
				for( int i = 0; i < iLengthData; ++i )
					sMnemonic[i] = static_cast<char>( m_pCPU->GetDataAtAdress( iAdress + i ) );

				break;
			}
			case 0x143:
			{
				sComment = ";CGB Flag";
				switch( m_pCPU->GetDataAtAdress( iAdress ) )
				{
					case 0:		sComment += " ;DMG - classic gameboy"; break;
					case 0x80:	sComment += " ;CGB - retro compat monochrome"; break;
					case 0xC0:	sComment += " ;CGB - only"; break;
				}
				break;
			}
			case 0x147:
			{
				sComment = ";Cartridge Type";
				switch( m_pCPU->GetDataAtAdress( iAdress ) )
				{
					case 0x00: sComment += " ROM ONLY"; break;
					case 0x01: sComment += " MBC1"; break;
					case 0x02: sComment += " MBC1+RAM"; break;
					case 0x03: sComment += " MBC1+RAM+BATTERY"; break;
					case 0x05: sComment += " MBC2"; break;
					case 0x06: sComment += " MBC2+BATTERY"; break;
					case 0x08: sComment += " ROM+RAM"; break;
					case 0x09: sComment += " ROM+RAM+BATTERY"; break;
					case 0x0B: sComment += " MMM01"; break;
					case 0x0C: sComment += " MMM01+RAM"; break;
					case 0x0D: sComment += " MMM01+RAM+BATTERY"; break;
					case 0x0F: sComment += " MBC3+TIMER+BATTERY"; break;
					case 0x10: sComment += " MBC3+TIMER+RAM+BATTERY"; break;
					case 0x11: sComment += " MBC3"; break;
					case 0x12: sComment += " MBC3+RAM"; break;
					case 0x13: sComment += " MBC3+RAM+BATTERY"; break;
					case 0x19: sComment += " MBC5"; break;
					case 0x1A: sComment += " MBC5+RAM"; break;
					case 0x1B: sComment += " MBC5+RAM+BATTERY"; break;
					case 0x1C: sComment += " MBC5+RUMBLE"; break;
					case 0x1D: sComment += " MBC5+RUMBLE+RAM"; break;
					case 0x1E: sComment += " MBC5+RUMBLE+RAM+BATTERY"; break;
					case 0x20: sComment += " MBC6"; break;
					case 0x22: sComment += " MBC7+SENSOR+RUMBLE+RAM+BATTERY"; break;
					case 0xFC: sComment += " POCKET CAMERA"; break;
					case 0xFD: sComment += " BANDAI TAMA5"; break;
					case 0xFE: sComment += " HuC3"; break;
					case 0xFF: sComment += " HuC1+RAM+BATTERY"; break;
					default:   sComment += " Unknown Type"; break;
				}
				break;
			}
			case 0x148:
			{
				sComment = ";ROM size";
				switch( m_pCPU->GetDataAtAdress( iAdress ) )
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
				break;
			}
			case 0x149:
			{
				sComment = ";RAM size";
				switch( m_pCPU->GetDataAtAdress( iAdress ) )
				{
					case 0x00: sComment += " 0, No RAM"; break;
					case 0x01: sComment += " -, Unused"; break;
					case 0x02: sComment += " 8 KiB, 1 bank"; break;
					case 0x03: sComment += " 32 KiB, 4 banks of 8 KiB each"; break;
					case 0x04: sComment += " 128 KiB, 16 banks of 8 KiB each"; break;
					case 0x05: sComment += " 64 KiB, 8 banks of 8 KiB each"; break;
					default:   sComment += " Unknown Code"; break;
				}
				break;
			}
			case 0x14A:
			{
				sComment = ";Destination code";
				if( m_pCPU->GetDataAtAdress( iAdress ) )
					sComment += " : Japanese";
				else
					sComment += " : Overseas only";
				break;
			}
			case 0x14E:
			{
				sComment = ";Global Checksum";
				iLengthData = 2;//TODO : Check that
				break;
			}
			case 0x13F: sComment = ";Manufacturer code"; break;
			case 0x144: sComment = ";New Licence Code"; break;
			case 0x146: sComment = ";SGB Flag"; break;
			case 0x14B: sComment = ";Old Licence code"; break;
			case 0x14C: sComment = ";Mask ROM version number"; break;
			case 0x14D: sComment = ";Header Checksum"; break;

			//case 0xFF00: sComment = ";Joypad"; break;
			//case 0xFF01: sComment = ";Serial data"; break;
			//case 0xFF02: sComment = ";Serial control"; break;
			//case 0xFF03: break;
			//case 0xFF04: sComment = ";Divider"; break;
			//case 0xFF05: sComment = ";Timer counter"; break;
			//case 0xFF06: sComment = ";Timer modulo"; break;
			//case 0xFF07: sComment = ";Timer ctrl"; break;
			//case 0xFF08: break;
			//case 0xFF0F: sComment = ";Int flag"; break;
			//case 0xFF30: sComment = ";Wave pattern"; break;
			//case 0xFF40: sComment = ";lcd ctrl"; break;
			//case 0xFF41: sComment = ";lcd stat"; break;
			//case 0xFF42: sComment = ";scroll Y"; break;
			//case 0xFF43: sComment = ";scroll X"; break;
			//case 0xFF44: sComment = ";LY"; break;
			//case 0xFF45: sComment = ";LYC"; break;
			//case 0xFF46: sComment = ";OAM DMA"; break;
			//case 0xFF47: sComment = ";bg pal"; break;
			//case 0xFF48: sComment = ";obj pal 0"; break;
			//case 0xFF49: sComment = ";obj pal 1"; break;
			//case 0xFF4A: sComment = ";win Y"; break;
			//case 0xFF4B: sComment = ";win X"; break;
			//case 0xFF4D: sComment = ";speed switch"; break;
			//case 0xFF4F: sComment = ";vram bank"; break;
			//case 0xFF50: sComment = ";disable bootrom"; break;
			//case 0xFF51: sComment = ";hdma src hi"; break;
			//case 0xFF52: sComment = ";hdma src low"; break;
			//case 0xFF53: sComment = ";hdma dest hi"; break;
			//case 0xFF54: sComment = ";hdma dest low"; break;
			//case 0xFF55: sComment = "; hdma count"; break;
			//case 0xFF56: sComment = ";ir port"; break;
			//case 0xFF68: sComment = ";bg pal set"; break;
			//case 0xFF69: sComment = ";bg pal data"; break;
			//case 0xFF6A: sComment = ";obj pal set"; break;
			//case 0xFF6B: sComment = ";obj pal data"; break;
			//case 0xFF70: sComment = ";wram bank"; break;
			//case 0xFF76: sComment = ";PCM34"; break;
			//case 0xFF77: sComment = ";PCM12"; break;

			default:
			{
				uint8_t iIndex = m_pCPU->GetDataAtAdress( iAdress );
				CPU::CPU_Instructions* pInstruction = iIndex == 0xCB ? CPU::m_aExtendOpcodesTable[ m_pCPU->GetDataAtAdress( iAdress + 1 ) ] : CPU::m_aOpcodesTable[ iIndex ];

				if( pInstruction )
				{
					g_iCounter += pInstruction->m_iDuration >> 2;
					iPCIncrease = pInstruction->m_iLength;
					iLengthData = pInstruction->m_iLength;

					sMnemonic = pInstruction->m_sMnemonic;
					iDuration = pInstruction->m_iDuration / 4; //  -> t cycle to m cycle
				}
				else
				{
					sMnemonic = "undefined opcode";
					aData[ 0 ] = 0;
				}
				break;
			}
		}
	}

	ToHex( iAdress,aData,iLengthData );
	WriteBlock( oData,iAdress,aData,sMnemonic.c_str(),sComment,iDuration);

	return iPCIncrease;
}

void Disassembler::ToHex( const uint16_t iSrcAdress,char* Dst,int len )
{
	//TODO: check len param
	static const char hex[] = "0123456789ABCDEF";
	for( size_t i = 0,k = 0; i < len; i++,k += 3 )
	{
		uint8_t val  = CPU::GetInstance()->GetDataAtAdress( iSrcAdress + i );
		Dst[ k + 0 ] = hex[ val >> 4 ];
		Dst[ k + 1 ] = hex[ val & 15 ];
		if( i + 1 < len )
			Dst[ k + 2 ] = ' ';
	}
}

bool SAXParser::boolean( bool val ) { return false; }
bool SAXParser::number_integer( number_integer_t val ) { return false; }
bool SAXParser::number_float( number_float_t val, const string_t& s ) { return false; }
bool SAXParser::binary( binary_t& val ){ return true; }
bool SAXParser::start_array( std::size_t elements ){ return true; }
bool SAXParser::end_array(){ return true;}												 //Not Used

bool SAXParser::null()
{
	return true;
}

bool SAXParser::number_unsigned( number_unsigned_t val )
{
	switch( m_iElementFlag )
	{
		case 0:
		{
			if( Disassembler::m_aDisassembly.empty() )
				Disassembler::m_aDisassembly.reserve( std::min<size_t>( static_cast<size_t>( val ), 100000 ) );
			break;
		}
		case 1:
			m_pLine->m_iAdress = val;
			break;
		case 5:
			m_pLine->m_iDuration = val;
			break;
		default :
			std::cerr << "Element not handle" << std::endl;
			return false;
	}
	return true;
}
bool SAXParser::string( string_t& val )
{
	switch( m_iElementFlag )
	{
		case 2:
			CopyTrunc( m_pLine->m_sMnemonic, val.c_str() );
			break;
		case 3:
			CopyTrunc( m_pLine->m_sAditionalInfo,val.c_str() );
			break;
		case 4:
			CopyTrunc( m_pLine->m_aData,val.c_str() );
			break;
		default:
			std::cerr << "Element not handle" << std::endl;
			return false;
	}
	return true;
}

bool SAXParser::end_object()
{
	--m_iDepth;
	m_iElementFlag = 0;
	return true;
}

bool SAXParser::key( string_t& val )
{
	if( val.size() == 1 )
	{
		switch( val[ 0 ] )
		{
			case 'a': m_iElementFlag = 1; return true;
			case 'm': m_iElementFlag = 2; return true;
			case 'c': m_iElementFlag = 3; return true;
			case 'b': m_iElementFlag = 4; return true;
			case 'd': m_iElementFlag = 5; return true;
		}
	}
	return true;
}

bool SAXParser::parse_error( std::size_t position, const std::string& last_token, const nlohmann::json::exception& ex )
{
	std::cerr << ex.what() << '\n';
	return false;
}

bool SAXParser::start_object( std::size_t elements )
{
	if( elements != 0 && elements != std::numeric_limits<std::size_t>::max() )
	{
		if( m_iDepth >= 1 )
		{
			auto& a = Disassembler::m_aDisassembly;
			a.emplace_back();
			m_pLine = &a.back();
		}

		++m_iDepth;
		return true;
	}

	return false;
}
