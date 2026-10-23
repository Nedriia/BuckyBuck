//
// Created by arthu on 16/07/2026.
//
#include "CPU.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include "Display.h"
#include <stdarg.h>

#ifdef DEBUG_INFO
#include "DebugInfosDisplay.h"
#endif

static constexpr uint8_t X_MASK = 0b11000000;
static constexpr uint8_t Y_MASK = 0b00111000;
static constexpr uint8_t P_MASK = 0b00110000;
static constexpr uint8_t Q_MASK = 0b00001000;
static constexpr uint8_t Z_MASK = 0b00000111;

static constexpr uint32_t r[ 8 ] = { 'B','C','D','E','H','L',' ','A' };
static constexpr std::string_view rp[ 4 ] = { "BC","DE","HL","SP" };
static constexpr std::string_view rp2[ 4 ] = { "BC","DE","HL","AF" };
static constexpr std::string_view cc[ 4 ] = { "NZ","Z","NC","C" };
static constexpr std::string_view alu[ 8 ] = { "ADD A,","ADC A,","SUB","SBC A,","AND","XOR","OR","CP" };
static constexpr std::string_view rot[ 8 ] = { "RLC","RRC","RL","RR","SLA","SRA","SWAP","SRL" };

CPU* CPU::m_pSingleton = nullptr;
CPU::CPU_Instructions* CPU::m_aOpcodesTable[ 256 ] = { nullptr };
CPU::CPU_Instructions* CPU::m_aExtendOpcodesTable[ 256 ] = { nullptr };
CPU::MemoryRegion CPU::m_aMemoryMap[12] = {
	{ 0x0000, 0x3FFF, "ROM" }, //16 KiB ROM bank 00
	{ 0x4000, 0x7FFF, "ROM" }, //16 KiB ROM Bank 01-NN
	{ 0x8000, 0x9FFF, "VRA" }, //8 KiB Video RAM (VRAM)
	{ 0xA000, 0xBFFF, "SRA" }, //8 KiB External RAM
	{ 0xC000, 0xCFFF, "WRA" }, //4 KiB Work RAM (WRAM)
	{ 0xD000, 0xDFFF, "WRA" }, //4 KiB Work RAM (WRAM)
	{ 0xE000, 0xFDFF, "ECHO" },//Echo RAM
	{ 0xFE00, 0xFE9F, "OAM" }, //Object Attribute Memory (OAM)
	{ 0xFEA0, 0xFEFF, "----" },//Not Usable
	{ 0xFF00, 0xFF7F, "I/O" }, //I/O Registers
	{ 0xFF80, 0xFFFE, "HRAM" },//High RAM (HRAM)
	{ 0xFFFF, 0xFFFF, "I/O" }  //Interrupt Enable register (IE)
};
std::string CPU::GetMemoryRegionLabel( const uint16_t iAdress )
{
	for ( int i = 0; i < 12; ++i )
	{
		if ( iAdress > m_aMemoryMap[i].m_iEndAdress )
			continue;

		if ( iAdress >= m_aMemoryMap[i].m_iStartAdress && iAdress <= m_aMemoryMap[i].m_iEndAdress )
			return m_aMemoryMap[i].m_sLabel;
	}
	return "";
}

CPU::CPU() :
	m_aMemory{0},
	m_iPC( 0 ),
	m_iSP( 0 ),
	m_iIR( 0 ),
	m_iIE( 0 ),
	m_RegisterAF( 0 ),
	m_RegisterBC( 0 ),
	m_RegisterDE( 0 ),
	m_RegisterHL( 0 ),
	ZFlag( false ),
	NFlag( false ),
	HFlag( false ),
	CFlag( false ),
	m_oCPUState( State::Pause )
{
	_FillOpcodesTables();
	Init();
}

CPU::~CPU()
{
	for( int i = 0; i < 256; ++i )
	{
		if( m_aOpcodesTable[ i ] != nullptr )
			delete m_aOpcodesTable[ i ];
		if( m_aExtendOpcodesTable[ i ] != nullptr )
			delete m_aExtendOpcodesTable[ i ];
	}

	m_pSingleton = nullptr;
}

void CPU::Init()
{
	m_iPC = 0;

	m_iSP = 0;
	m_iIR = 0;
	m_iIE = 0;

	m_RegisterAF.reg = 0;
	m_RegisterBC.reg = 0;
	m_RegisterDE.reg = 0;
	m_RegisterHL.reg = 0;

	ZFlag = false;
	NFlag = false;
	HFlag = false;
	CFlag = false;

	//TODO : Re Init memory ? 

	m_oCPUState = State::Pause;
}

void CPU::_SimulateBootRom()
{
	m_RegisterAF.hi = 0x11;
	m_RegisterAF.lo = 1 << 7;

	m_RegisterBC.hi = 0x00;
	m_RegisterBC.lo = 0x00;

	m_RegisterDE.hi = 0xFF;
	m_RegisterDE.lo = 0x56;

	m_RegisterHL.hi = 0x00;
	m_RegisterHL.lo = 0x0D;

	m_iPC = 0x100;
	m_iSP = 0xFFFE;
}

void CPU::_UpdateFlags()
{
	ZFlag = ( m_RegisterAF.lo >> 7 ) & 1; //Zero flag
	NFlag = ( m_RegisterAF.lo >> 6 ) & 1; //Subtraction flag( BCD )
	HFlag = ( m_RegisterAF.lo >> 5 ) & 1; //Half Carry flag( BCD )
	CFlag = ( m_RegisterAF.lo >> 4 ) & 1; //Carry flag
}

int CPU::LoadRom( const char* sROMPath )
{
	_SimulateBootRom();
	_UpdateFlags();

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

		m_aMemory.resize( size );
		memcpy( m_aMemory.data(), memblock, size );
#ifdef DEBUG_INFO
		DebugInfosDisplay::GetHexEditor()->LoadBufferFromMemory( m_aMemory );
#endif
		delete[] memblock;
	}
	else
	{
		std::cerr << "ERROR::ROM::LOADING::FILE_NOT_FOUND " << sRomPath  << std::endl;
		return -1;
	}
	return 0;
}

void CPU::FetchDecode()
{
	if( m_aOpcodesTable[ GetDataAtAdress( m_iPC ) ] != nullptr )
	{
		CPU_Instructions* pInstruction = m_aOpcodesTable[ GetDataAtAdress( m_iPC ) ];

		std::cout << pInstruction->m_sMnemonic << " " << static_cast< int >( pInstruction->m_iLength ) << " " << static_cast< int >( pInstruction->m_iDuration ) << " ";
		if ( pInstruction->m_iConditionalDuration != 0 )
			std::cout << ( int )pInstruction->m_iConditionalDuration;

		uint16_t iPreviousPC = m_iPC;
		( this->*pInstruction->m_pFct )();
		std::cout << std::endl;

		if( iPreviousPC == m_iPC )
			m_iPC += pInstruction->m_iLength;
	}
	else
		std::cout << "UNKNOWN CPU INSTR" << std::endl;
}

void CPU::DestroyInstance()
{
	delete m_pSingleton;
}

void CPU::Reset()
{
	m_oCPUState = State::Reset;
	_SimulateBootRom();
	_UpdateFlags();
	m_oCPUState = State::Pause;
}

uint8_t CPU::GetDataAtAdress( const uint16_t iAdress ) const
{
	if( iAdress < 0 || iAdress >= GetMemorySize() )
		return 0;

	return m_aMemory[ iAdress ];
}

uint8_t& CPU::GetRefAtMemory( const uint16_t iAdress )
{
	return m_aMemory.at( iAdress );
}

void CPU::AddCPUInstruction( const uint8_t iIndex, const fct_opcode& pFct, uint8_t iFlags, uint8_t iFlagSet1, uint8_t iFlagReset0, std::array<uint8_t, 3> aValues, bool bExtent, const char* sMnemonic, ... )
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

	pInstruction->m_pFct = pFct;
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

void CPU::EmulateCycle()
{
	if( m_oCPUState == State::Running || m_oCPUState == State::SkipToNext )
		FetchDecode();

	if( m_oCPUState == State::SkipToNext )
		m_oCPUState = State::Pause;
}

void CPU::LD_r16n16()
{
	uint16_t iValue = ( GetDataAtAdress( m_iPC + 2 ) << 8 ) | GetDataAtAdress( m_iPC + 1 );
	std::cout << std::hex << std::uppercase << " ( 0X" << ( int )iValue << " )" << std::dec;

	_LoadR16Value( _Get_Destr16(), iValue );
}

void CPU::LD_r8r8()
{
	_LoadR8Value( _Get_Destr8(),_Get_Destr8( true ) );
}

void CPU::LD_A_HLI()
{
	_LoadR8Value( m_RegisterAF.hi, GetRefAtMemory( m_RegisterHL.reg ) );
	m_RegisterHL.reg++;
}

void CPU::LD_A_HLD()
{
	_LoadR8Value( m_RegisterAF.hi, GetRefAtMemory( m_RegisterHL.reg ) );
	m_RegisterHL.reg--;
}

void CPU::LD_r8d8()
{
	_LoadR8Value( _Get_Destr8(),GetRefAtMemory( m_iPC + 1 ) );
}

void CPU::LD_r16A()
{
	uint8_t& rA = m_RegisterAF.hi;
	_LoadR8Value( rA,GetRefAtMemory( _Get_Destr16( true ) ) );
}

void CPU::JPn16()
{
	//Read next 2 bytes of memory
	uint16_t iAdress = ( GetDataAtAdress( m_iPC + 2 ) << 8 ) | GetDataAtAdress( m_iPC + 1 );
	std::cout << std::hex << std::uppercase << " ( 0X" << ( int )iAdress << " )" << std::dec;
	m_iPC = iAdress;
}

void CPU::INC_r8()
{
	uint16_t iAdress = GetDataAtAdress( m_iPC );
	uint8_t y = ( iAdress & Y_MASK ) >> 3;

	//_INCRegisterVal( y );
	//FLAGS
}

void CPU::JR_cc_n16()
{
	uint16_t iAdress = GetDataAtAdress( m_iPC );
	uint8_t z = ( iAdress & Z_MASK );

	int8_t iValue = static_cast<int8_t>( GetDataAtAdress( m_iPC + 1 ) );
	if ( !z )
		m_iPC += iValue;
}

void CPU::_FillOpcodesTables()
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
					case 0: CPU::AddCPUInstruction( i,{ &CPU::NOP }, 0,0,0,{1,4},false,"NOP" ); break;
					case 1: CPU::AddCPUInstruction( i,{ &CPU::LD },  0,0,0,{3,20},false,"LD a16, SP" ); break;
					case 2: CPU::AddCPUInstruction( i,{ &CPU::STOP },0,0,0,{2,4},false,"STOP" ); break;
					case 3: CPU::AddCPUInstruction( i,{ &CPU::JR },  0,0,0,{2,12},false,"JR s8" ); break;
					case 4:
					case 5:
					case 6:
					case 7:
						CPU::AddCPUInstruction( i,{ &CPU::JR_cc_n16 },0,0,0,{2,12,8},false,"JR %s s8",cc[ y-4 ].data() ); break;
				}
				break;
			}
			case 1:
			{
				switch( q )
				{
					case 0: CPU::AddCPUInstruction( i,{ &CPU::LD_r16n16 },0,0,0,{3,12},false,"LD %s, d16",rp[ p ].data() ); break;
					case 1: CPU::AddCPUInstruction( i,{ &CPU::ADD },static_cast< uint8_t >( N | H | C ), (0), (N),{1,8},false,"ADD HL, %s",rp[ p ].data() ); break;
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
						case 0: CPU::AddCPUInstruction( i,{ &CPU::LD_r16A },0,0,0,{1,8},false,"LD BC, A" ); break;
						case 1: CPU::AddCPUInstruction( i,{ &CPU::LD_r16A },0,0,0,{1,8},false,"LD DE, A" ); break;
						case 2: CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD [HL+], A" ); break;
						case 3: CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD [HL-], A" ); break;
					}
					break;
				}
				case 1:
				{
					switch( p )
					{
						case 0: CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD A, BC" ); break;
						case 1: CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD A, DE" ); break;
						case 2: CPU::AddCPUInstruction( i,{ &CPU::LD_A_HLI },0,0,0,{1,8},false,"LD A, [HL+]" ); break;
						case 3: CPU::AddCPUInstruction( i,{ &CPU::LD_A_HLD },0,0,0,{1,8},false,"LD A, [HL-]" ); break;
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
				case 0: CPU::AddCPUInstruction( i,{ &CPU::INC },0,0,0,{1,8},false,"INC %s",rp[ p ].data() ); break;
				case 1: CPU::AddCPUInstruction( i,{ &CPU::DEC },0,0,0,{1,8},false,"DEC, %s",rp[ p ].data() ); break;
				}
				break;
			}
			case 4:
			{
				if( y == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::INC },( Z | N | H ),0, ( N ),{1,12},false,"INC HL" );
				else
					CPU::AddCPUInstruction( i,{ &CPU::INC_r8 },( Z | N | H ),0, ( N ),{1,4},false,"INC %c",r[ y ] );
				break;
			}
			case 5:
			{
				if( y == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::DEC },(Z | N | H),( N ), 0,{1,12},false,"DEC HL" );
				else
					CPU::AddCPUInstruction( i,{ &CPU::DEC },(Z | N | H),( N ), 0,{1,4},false,"DEC, %c",r[ y ] );
				break;
			}
			case 6:
			{
				if( y == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{2,12},false,"LD HL, d8" );
				else
					CPU::AddCPUInstruction( i,{ &CPU::LD_r8d8 },0,0,0,{2,8},false,"LD %c, d8",r[ y ] );
				break;
			}
			case 7:
			{
				switch( y )
				{
					case 0: CPU::AddCPUInstruction( i,{ &CPU::RLCA },( Z | N | H | C ),  0,		 ( Z | N | H ), {1,4},false,"RLCA" ); break;
					case 1: CPU::AddCPUInstruction( i,{ &CPU::RRCA },( Z | N | H | C ),  0,		 ( Z | N | H ), {1,4},false,"RRCA" ); break;
					case 2: CPU::AddCPUInstruction( i,{ &CPU::RLA }, ( Z | N | H | C ),  0,		 ( Z | N | H ), {1,4},false,"RLA" ); break;
					case 3: CPU::AddCPUInstruction( i,{ &CPU::RRA }, ( Z | N | H | C ),  0,		 ( Z | N | H ), {1,4},false,"RRA" ); break;
					case 4: CPU::AddCPUInstruction( i,{ &CPU::DAA }, ( Z     | H | C ),  0,		 (		   H ),	{1,4},false,"DAA" ); break;
					case 5: CPU::AddCPUInstruction( i,{ &CPU::CPL }, (	   N | H	), ( N | H	),		   0,	{1,4},false,"CPL" ); break;
					case 6: CPU::AddCPUInstruction( i,{ &CPU::SCF }, (     N | H | C ),( C ),	 (	   N | H ),	{1,4},false,"SCF" ); break;
					case 7: CPU::AddCPUInstruction( i,{ &CPU::CCF }, (     N | H | C ),  0,		 (     N | H ),	{1,4},false,"CCF" ); break;
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
				CPU::AddCPUInstruction( i,{ &CPU::HALT },0,0,0,{1,4},false,"HALT" );
			}
			else
			{
				if( y == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD HL, %c",r[ z ] );
				else if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD %c, HL",r[ y ] );
				else
					CPU::AddCPUInstruction( i,{ &CPU::LD_r8r8 },0,0,0,{1,4},false,"LD %c, %c",r[ y ],r[ z ] );
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
						CPU::AddCPUInstruction( i,{ &CPU::ADD },( Z | N | H | C ),0,( N ),{1,8},false,"%s HL", alu[ y ].data(),r[ z ] );
					else
						CPU::AddCPUInstruction( i,{ &CPU::ADD },( Z | N | H | C ),0,( N ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 1:
				{
					if ( z == 6 )
						CPU::AddCPUInstruction( i,{ &CPU::ADC },( Z | N | H | C ),0,( N ),{1,8},false, "%s HL", alu[ y ].data(),r[ z ] );
					else
						CPU::AddCPUInstruction( i,{ &CPU::ADC },( Z | N | H | C ),0,( N ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 2:
				{
					if ( z == 6 )
						CPU::AddCPUInstruction( i,{ &CPU::SUB },( Z | N | H | C ),( N ), 0,{1,8},false, "%s HL", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							CPU::AddCPUInstruction( i,{ &CPU::SUB },( Z | N | H | C ),( Z | N ),( H | C ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
						else
							CPU::AddCPUInstruction( i,{ &CPU::SUB },( Z | N | H | C ),(		N ),	0	 ,{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 3:
				{
					if ( z == 6 )
						CPU::AddCPUInstruction( i,{ &CPU::SBC },	( Z | N | H | C ),( N ), 0,	{1,8},false, "%s HL", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							CPU::AddCPUInstruction( i,{ &CPU::SBC },( Z | N | H ),	  ( N ), 0,	{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							CPU::AddCPUInstruction( i,{ &CPU::SBC },( Z | N | H | C ),( N ), 0,	{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 4:
				{
					if ( z == 6 )
						CPU::AddCPUInstruction( i,{ &CPU::AND },( Z | N | H | C ), ( H ),( N | C ),{1,8},false, "%s HL", alu[ y ].data(),r[ z ] );
					else
						CPU::AddCPUInstruction( i,{ &CPU::AND },( Z | N | H | C ), ( H ),( N | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 5:
				{
					if ( z == 6 )
						CPU::AddCPUInstruction( i,{ &CPU::XOR },	( Z | N | H | C ), ( Z ),( N | H | C ),{1,8},false, "%s HL", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							CPU::AddCPUInstruction( i,{ &CPU::XOR },( Z | N | H | C ), ( Z ),( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							CPU::AddCPUInstruction( i,{ &CPU::XOR },( Z | N | H | C ),	 0,	 ( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 6:
				{
					if ( z == 6 )
						CPU::AddCPUInstruction( i,{ &CPU::OR },( Z | N | H | C ),	0,	( N | H | C ),{1,8},false, "%s HL", alu[ y ].data(),r[ z ] );
					else
						CPU::AddCPUInstruction( i,{ &CPU::OR },( Z | N | H | C ), ( Z ),( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 7:
				{
					if ( z == 6 )
						CPU::AddCPUInstruction( i,{ &CPU::CP },		( Z | N | H | C ),		0,	( N ),			{1,8},false, "%s HL", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							CPU::AddCPUInstruction( i,{ &CPU::CP }, ( Z | N | H | C ), ( Z | N ),	( H | C ),	{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							CPU::AddCPUInstruction( i,{ &CPU::CP }, ( Z | N | H | C ), (	 N ),	0,			{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
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
							CPU::AddCPUInstruction( i,{ &CPU::RET },	0,			  0,	0,		{1,20,8},false,"RET %s",cc[ y ].data() ); break;
					case 4: CPU::AddCPUInstruction( i,{ &CPU::LDH_C },	0,			  0,	0,		{2,12},false,"LD (0xFF00+C), A" ); break;
					case 5: CPU::AddCPUInstruction( i,{ &CPU::ADD },( Z | N | H | C ),0,( Z | N ),  {2,16},false,"ADD SP, s8" ); break;
					case 6: CPU::AddCPUInstruction( i,{ &CPU::LDH },	0,			  0,	0,		{2,8},false,"LD A, (0xFF00+C)" ); break;
					case 7: CPU::AddCPUInstruction( i,{ &CPU::LD }, ( Z | N | H | C ),0,( Z | N ),  {2,12},false,"LD HL, SP+s8" ); break;
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
						CPU::AddCPUInstruction( i,{ &CPU::POP },( Z | N | H | C ),0,0,{1,12},false,"POP %s",rp2[ p ].data() );
					else
						CPU::AddCPUInstruction( i,{ &CPU::POP },	  0,		  0,0,{1,12},false,"POP %s",rp2[ p ].data() );
					break;
				}
				case 1:
				{
					switch( p )
					{
						case 0: CPU::AddCPUInstruction( i,{ &CPU::RET }, 0,0,0,{1,16},false,"RET" ); break;
						case 1: CPU::AddCPUInstruction( i,{ &CPU::RETI },0,0,0,{1,16},false,"RETI" ); break;
						case 2: CPU::AddCPUInstruction( i,{ &CPU::JP },  0,0,0,{1,4},false,"JP HL" ); break;
						case 3: CPU::AddCPUInstruction( i,{ &CPU::LD },  0,0,0,{1,8},false,"LD SP, HL" ); break;
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
							CPU::AddCPUInstruction( i,{ &CPU::JP },0,0,0,{3,16,12},false,"JP %s a16",cc[ y ].data() ); break;
					case 4: CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD [C], A",cc[ y ].data() ); break;
					case 5: CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{3,16},false,"LD a16, A" ); break;
					case 6: CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{1,8},false,"LD A, [C]" ); break;
					case 7: CPU::AddCPUInstruction( i,{ &CPU::LD },0,0,0,{3,16},false,"JP A, a16" ); break;
				}
				break;
			}
			case 3:
			{
				switch( y )
				{
				case 0: CPU::AddCPUInstruction( i,{ &CPU::JPn16 },0,0,0,{3,16},false,"JP a16" ); break;
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
								case 0: CPU::AddCPUInstruction( k,{ &CPU::RLC }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 1: CPU::AddCPUInstruction( k,{ &CPU::RRC }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 2: CPU::AddCPUInstruction( k,{ &CPU::RL },  ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 3: CPU::AddCPUInstruction( k,{ &CPU::RR },  ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 4: CPU::AddCPUInstruction( k,{ &CPU::SLA }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 5: CPU::AddCPUInstruction( k,{ &CPU::SRA }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 6: CPU::AddCPUInstruction( k,{ &CPU::SWAP },( Z | N | H | C ), 0,( N | H | C ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 7: CPU::AddCPUInstruction( k,{ &CPU::SRL }, ( Z | N | H | C ), 0,( N | H ),	 { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
							}
							break;
						}
						case 1:
						{
							if( z == 6 )
								CPU::AddCPUInstruction( k,{ &CPU::BIT },( Z | N | H ), ( H ),( N ),{2,12},true,"BIT %i, HL",y );
							else
								CPU::AddCPUInstruction( k,{ &CPU::BIT },( Z | N | H ), ( H ),( N ),{2,8},true,"BIT %i, %c",y,r[ z ] );
							break;
						}
						case 2:
						{
							if( z == 6 )
								CPU::AddCPUInstruction( k,{ &CPU::RES },0,0,0,{2,16},true,"RES %i, HL",y );
							else
								CPU::AddCPUInstruction( k,{ &CPU::RES },0,0,0,{2,8},true,"RES %i, %c",y,r[ z ] );
							break;
						}
						case 3:
						{
							if( z == 6 )
								CPU::AddCPUInstruction( k,{ &CPU::SET },0,0,0,{2,16},true,"SET %i, HL",y );
							else
								CPU::AddCPUInstruction( k,{ &CPU::SET },0,0,0,{2,8},true,"SET %i, %c",y,r[ z ] );
						}
						}
					}
					break;
				}
				case 6: CPU::AddCPUInstruction( i,{ &CPU::DI },0,0,0,{1,4},false,"DI" ); break;
				case 7: CPU::AddCPUInstruction( i,{ &CPU::EI },0,0,0,{1,4},false,"EI" ); break;
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
						CPU::AddCPUInstruction( i,{ &CPU::PUSH },0,0,0,{3,24,12},false,"CALL %s, a16",cc[ y ].data() ); break;
				}
				break;
			}
			case 5:
			{
				if( q == 0 )
					CPU::AddCPUInstruction( i,{ &CPU::PUSH },0,0,0,{1,16},false,"PUSH %s",rp2[ p ].data() );
				else if( q == 1 && p == 0 )
					CPU::AddCPUInstruction( i,{ &CPU::CALL },0,0,0,{3,24},false,"CALL a16" );
				break;
			}
			case 6:
			{
				switch( y )
				{
					case 0: CPU::AddCPUInstruction( i,{ &CPU::ADD },( Z | N | H | C ),   0,	  ( N ),		{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 1: CPU::AddCPUInstruction( i,{ &CPU::ADC },( Z | N | H | C ),   0,	  ( N ),		{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 2: CPU::AddCPUInstruction( i,{ &CPU::SUB },( Z | N | H | C ), ( N ),   0,			{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 3: CPU::AddCPUInstruction( i,{ &CPU::SBC },( Z | N | H | C ), ( N ),   0,			{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 4: CPU::AddCPUInstruction( i,{ &CPU::AND },( Z | N | H | C ), ( H ), ( N | C ),	{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 5: CPU::AddCPUInstruction( i,{ &CPU::XOR },( Z | N | H | C ),   0,	  ( N | H | C ),{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 6: CPU::AddCPUInstruction( i,{ &CPU::OR }, ( Z | N | H | C ),   0,	  ( N | H | C ),{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 7: CPU::AddCPUInstruction( i,{ &CPU::CP }, ( Z | N | H | C ), ( N ),   0,			{2,8},false,"%s d8",alu[ y ].data() ); break;
				}
				break;
			}
			case 7: CPU::AddCPUInstruction( i,{ &CPU::RST },0,0,0,{1,16},false,"RST %02X",y * 8 ); break;
			}
			break;
		}
		default: std::cout << "Case not handled" << std::endl; break;
		}
	}
}

uint8_t& CPU::_Get_Destr8( bool s8 /*= false */)
{
	const uint8_t iData = GetDataAtAdress( m_iPC );
	uint8_t r8 = s8 ? ( iData & Z_MASK ) : ( iData & Y_MASK ) >> 3;
	switch( r8 )
	{
		case 0: return m_RegisterBC.hi;						break;
		case 1: return m_RegisterBC.lo;						break;
		case 2: return m_RegisterDE.hi;						break;
		case 3: return m_RegisterDE.lo;						break;
		case 4: return m_RegisterHL.hi;						break;
		case 5: return m_RegisterHL.lo;						break;
		case 6: return GetRefAtMemory( m_RegisterHL.reg );	break;
		case 7: return m_RegisterAF.hi;						break;
	}
	throw std::runtime_error( "ERROR_GETTING_DESTR8" );
}

uint16_t& CPU::_Get_Destr16( bool bMem /*= false*/ )
{
	uint8_t r8 = ( GetDataAtAdress( m_iPC ) & P_MASK ) >> 4;
	switch( r8 )
	{
		case 0: return m_RegisterBC.reg;							break;
		case 1: return m_RegisterDE.reg;							break;
		case 2: 
		{
			if( bMem )
				m_RegisterAF.hi++;
			return m_RegisterHL.reg;								break;
		}
		case 3:
		{
			if( bMem )
			{
				m_RegisterAF.hi--;
				return m_RegisterHL.reg;
			}
			return m_iSP;											break;
		}
	}
	throw std::runtime_error( "ERROR_GETTING_DESTR16" );
}

void CPU::_LoadR8Value( uint8_t& iDst, uint8_t iData )
{
	iDst = iData;
}

void CPU::_LoadR16Value( uint16_t& iDst,uint16_t iData )
{
	iDst = iData;
}