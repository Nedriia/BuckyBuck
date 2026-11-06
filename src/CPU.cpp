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
#include "Timer.h"
#include "PPU.h"

static bool m_bJumped = false;
static constexpr uint8_t X_MASK  = 0b11000000;
static constexpr uint8_t Y_MASK  = 0b00111000;
static constexpr uint8_t P_MASK  = 0b00110000;
static constexpr uint8_t Q_MASK  = 0b00001000;
static constexpr uint8_t Z_MASK  = 0b00000111;
static constexpr uint8_t CC_MASK = 0b00011000;

static constexpr uint32_t r[ 8 ] = { 'B','C','D','E','H','L',' ','A' };
static constexpr std::string_view rp[ 4 ] = { "BC","DE","HL","SP" };
static constexpr std::string_view rp2[ 4 ] = { "BC","DE","HL","AF" };
static constexpr std::string_view cc[ 4 ] = { "NZ","Z","NC","C" };
static constexpr std::string_view alu[ 8 ] = { "ADD A,","ADC A,","SUB","SBC A,","AND","XOR","OR","CP" };
static constexpr std::string_view rot[ 8 ] = { "RLC","RRC","RL","RR","SLA","SRA","SWAP","SRL" };

namespace interrupts
{
	const uint16_t vblank = 0x40;
	const uint16_t lcdc_status = 0x48;
	const uint16_t timer = 0x50;
	const uint16_t serial = 0x58;
	const uint16_t joypad = 0x60;
}

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
const std::string& CPU::GetMemoryRegionLabel( const uint16_t iAdress )
{
	for ( int i = 0; i < 12; ++i )
	{
		if ( iAdress > m_aMemoryMap[i].m_iEndAdress )
			continue;

		if ( iAdress >= m_aMemoryMap[i].m_iStartAdress && iAdress <= m_aMemoryMap[i].m_iEndAdress )
			return m_aMemoryMap[i].m_sLabel;
	}
	throw std::runtime_error( "ADRESS_NOT_VALID" );
}

CPU::Register CPU::GetRegister( REGISTER e ) const
{
	switch( e )
	{
		case AF : return m_RegisterAF; break;
		case BC : return m_RegisterBC; break;
		case DE : return m_RegisterDE; break;
		case HL : return m_RegisterHL; break;
	}
	throw std::runtime_error( "REGISTER_NOT_VALID" );
}

CPU::CPU() :
	m_aROM_Memory{0},
	m_iPC( 0 ),
	m_iSP( 0 ),
	m_iIR( 0 ),
	m_RegisterAF( 0 ),
	m_RegisterBC( 0 ),
	m_RegisterDE( 0 ),
	m_RegisterHL( 0 ),
	m_bIME( false ),
	m_bHalted( false ),
	m_IME_Requested( -1 ),
	m_iLineCounter( 0 ),
	m_oCPUState( State::Pause ),
	m_pCurrentInstruction( nullptr )
{
	m_aMemory = std::vector<uint8_t>( 0x10000,0xFF );

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

	PPU::GetInstance()->Destroy();
	m_pSingleton = nullptr;
}

void CPU::Init()
{
	m_iPC = 0;

	m_iSP = 0;
	m_iIR = 0;

	m_RegisterAF.reg = 0;
	m_RegisterBC.reg = 0;
	m_RegisterDE.reg = 0;
	m_RegisterHL.reg = 0;

	m_bIME	  = false;
	m_bHalted = false;
	m_IME_Requested = -1;
	m_iLineCounter = 0;

	memset( m_aMemory.data(),0xFF,0x10000 );
	m_oCPUState = State::Pause;

	m_pCurrentInstruction = nullptr;

	m_bJumped = false;
}

void CPU::_SimulateBootRom()
{
	m_RegisterAF.hi = 0x01;

	m_RegisterBC.hi = 0xFF;
	m_RegisterBC.lo = 0x13;

	m_RegisterDE.hi = 0x00;
	m_RegisterDE.lo = 0xC1;

	m_RegisterHL.hi = 0x84;
	m_RegisterHL.lo = 0x03;

	m_iPC = 0x100;
	m_iSP = 0xFFFE;

	//(I/O) & Controls ---
	WriteMemory( 0xFF00,0xCF ); // P1 (Joypad)
	WriteMemory( 0xFF01,0x00 ); // SB (Serial transfer data)
	WriteMemory( 0xFF02,0x7E ); // SC (Serial transfer control)
	GetRefAtMemory( 0xFF04 ) = 0x18; // DIV (Divider register)
	WriteMemory( 0xFF05,0x00 ); // TIMA
	WriteMemory( 0xFF06,0x00 ); // TMA
	WriteMemory( 0xFF07,0xF8 ); // TAC
	WriteMemory( 0xFF0F,0xE1 ); // IF (Interrupt Flag)

	// --- Audio (APU) ---
	WriteMemory( 0xFF10,0x80 ); // NR10
	WriteMemory( 0xFF11,0xBF ); // NR11
	WriteMemory( 0xFF12,0xF3 ); // NR12
	WriteMemory( 0xFF13,0xFF ); // NR13
	WriteMemory( 0xFF14,0xBF ); // NR14
	WriteMemory( 0xFF16,0x3F ); // NR21
	WriteMemory( 0xFF17,0x00 ); // NR22
	WriteMemory( 0xFF18,0xFF ); // NR23
	WriteMemory( 0xFF19,0xBF ); // NR24
	WriteMemory( 0xFF1A,0x7F ); // NR30
	WriteMemory( 0xFF1B,0xFF ); // NR31
	WriteMemory( 0xFF1C,0x9F ); // NR32
	WriteMemory( 0xFF1D,0xFF ); // NR33
	WriteMemory( 0xFF1E,0xBF ); // NR34
	WriteMemory( 0xFF20,0xFF ); // NR41
	WriteMemory( 0xFF21,0x00 ); // NR42
	WriteMemory( 0xFF22,0x00 ); // NR43
	WriteMemory( 0xFF23,0xBF ); // NR44
	WriteMemory( 0xFF24,0x77 ); // NR50
	WriteMemory( 0xFF25,0xF3 ); // NR51
	WriteMemory( 0xFF26,0xF1 ); // NR52 (Audio activ�)

	// --- Vid�o (LCD / PPU) ---
	WriteMemory( 0xFF40,0x91 ); // LCDC
	WriteMemory( 0xFF41,0x81 ); // STAT
	WriteMemory( 0xFF42,0x00 ); // SCY
	WriteMemory( 0xFF43,0x00 ); // SCX
	WriteMemory( 0xFF44,0x91 ); // LY
	WriteMemory( 0xFF45,0x00 ); // LYC
	WriteMemory( 0xFF46,0xFF ); // DMA
	WriteMemory( 0xFF47,0xFC ); // BGP
	WriteMemory( 0xFF48,0x00 ); // OBP0
	WriteMemory( 0xFF49,0x00 ); // OBP1
	WriteMemory( 0xFF4A,0x00 ); // WY
	WriteMemory( 0xFF4B,0x00 ); // WX

	// --- Interruptions ---
	WriteMemory( 0xFFFF,0x00 ); // IE (Interrupt Enable)
}

void CPU::_SetFlag( CPU_FLAGS eFlag,bool bCond )
{
	uint8_t iMask = 0;
	switch( eFlag )
	{
		case ZERO_FLAG:			iMask = 1 << 7; break;
		case SUB_FLAG:			iMask = 1 << 6; break;
		case HALF_CARRY_FLAG:	iMask = 1 << 5; break;
		case CARRY_FLAG:		iMask = 1 << 4; break;
	}

	if( bCond )
		m_RegisterAF.lo |= iMask;
	else
		m_RegisterAF.lo &= ~iMask;
}

bool CPU::_HandleInterupts()
{
	// [IF] bit are only requesting an interrupt
	uint8_t IF = ReadMemory( 0xFF0F );
	uint8_t IE = ReadMemory( 0xFFFF );
	uint8_t iBitsOn = ( IF & IE & 0x1F );

	if( m_IME_Requested == 0 )
	{
		m_bIME = true;
		m_IME_Requested = -1;
	}
	else if( m_IME_Requested != -1 )
		m_IME_Requested--;

	if( m_bIME == true )
	{
		//Actual execution happens if IME & same bit in the IE are set
		if( iBitsOn )
		{
			if( m_bHalted )
				m_bHalted = false;

			if( _CheckInterupts( 0, interrupts::vblank, iBitsOn ) != 0 )	return true;
			if( _CheckInterupts( 1, interrupts::lcdc_status, iBitsOn ) )	return true;
			if( _CheckInterupts( 2, interrupts::timer, iBitsOn ) )			return true;
			if( _CheckInterupts( 3, interrupts::serial, iBitsOn ) )			return true;
			if( _CheckInterupts( 4, interrupts::joypad, iBitsOn ) )			return true;
		}
	}
	else
	{
		if( iBitsOn && m_bHalted )
		{
			//HALT Bug
			m_bHalted = false;
		}
	}
	return false;
}

bool CPU::_CheckInterupts( const uint8_t iInterruptBit, const uint16_t iInterruptAdress, const uint8_t iBitsOn )
{
	if( ( iBitsOn & ( 1 << iInterruptBit ) ) != 0 )
	{
		--m_iSP;
		WriteMemory( m_iSP,m_iPC >> 8 );
		--m_iSP;
		WriteMemory( m_iSP,m_iPC & 0x00FF );

		WriteMemory( 0xFF0F, ReadMemory( 0xFF0F ) & ~( 1 << iInterruptBit ) );
		m_iPC = iInterruptAdress;
		m_bIME = false;
		return true;
	}
	else
		return false;
}

int CPU::LoadRom( const char* sROMPath )
{
	_SimulateBootRom();

	std::string sRomPath = std::string( PATH_ROMS ) + sROMPath;
	std::ifstream file( sRomPath,std::ios::binary | std::ios::in | std::ios::ate );
	if( file.is_open() )
	{
		std::streamsize size = file.tellg();
		char* memblock = nullptr;
		std::streamsize bytesRead = 0;
		try
		{
			if( size <= 0 )
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

		m_aROM_Memory.resize( size );
		memcpy( m_aROM_Memory.data(), memblock, size );
		memcpy( m_aMemory.data(),memblock,size > 0x8000 ? 0x8000 : size);

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

uint8_t CPU::ExecuteNextOpcode()
{
	if( _HandleInterupts() )
		return 20;

	if( m_bHalted ) return 4;

	return _FetchDecode();
}

uint8_t CPU::_FetchDecode()
{
	uint8_t iIndex = ReadMemory( m_iPC );
	if( iIndex == 0xCB )
	{
		m_iPC++;
		m_pCurrentInstruction = CPU::m_aExtendOpcodesTable[ ReadMemory( m_iPC ) ];
	}
	else
	{
		m_pCurrentInstruction = CPU::m_aOpcodesTable[ iIndex ];
	}

	if( m_pCurrentInstruction != nullptr )
	{
		m_bJumped = false;
		( this->*m_pCurrentInstruction->m_pFct )( );
		if( !m_bJumped )
			m_iPC += m_pCurrentInstruction->m_iLength - ( iIndex == 0xCB ? 1 : 0 );

		return m_pCurrentInstruction->m_iDuration;
	}
	else
		throw std::runtime_error( "UNKNOWN_CPU_INSTR??" );
}

void CPU::DestroyInstance()
{
	delete m_pSingleton;
}

void CPU::Reset()
{
	m_oCPUState = State::Reset;
	Init();

	size_t size = m_aROM_Memory.size();
	if( size > 0 )
		memcpy( m_aMemory.data(),m_aROM_Memory.data(),size > 0x8000 ? 0x8000 : size );

	_SimulateBootRom();
}

void CPU::MarkBreakpoint( const uint16_t iAdress )
{
	if( m_aBreakpoints.find( iAdress ) == m_aBreakpoints.end() )
		m_aBreakpoints.insert( iAdress );
	else
		m_aBreakpoints.erase( iAdress );
}

bool CPU::IsBreakpoint( const uint16_t iAdress ) const
{
	return m_aBreakpoints.find( iAdress ) != m_aBreakpoints.end();
}

void CPU::RequestInterrupt( uint8_t iBit )
{
	WriteMemory( 0xFF0F, ReadMemory( 0xFF0F ) | ( 1 << iBit ) );
}

uint8_t CPU::ReadMemory( const uint16_t iAdress ) const
{
	if( iAdress >= 0xFEA0 && iAdress <= 0xFEFF )
		return 0xFF;

	return m_aMemory.at( iAdress );
}

void CPU::WriteMemory( const uint16_t iAdress,const uint8_t iValue )
{
	if( iAdress >= 0xFEA0 && iAdress <= 0xFEFF )
		return;

	if( iAdress == 0xFF02 && iValue == 0x81 )
	{
		std::cout << static_cast< char >( m_aMemory[ 0xFF01 ] ) << std::flush;
		m_aMemory[ 0xFF02 ] = 0x01;
		return;
	}

	if( iAdress == 0xFF04 )//Divider Register
	{
		m_aMemory.at( iAdress ) = 0x0;
		Timer::GetInstance()->SetResetDivider();
		return;
	}
	else if( iAdress == 0xFF07 )
	{
		m_aMemory.at( iAdress ) = iValue;
		Timer::GetInstance()->SetClockFrequency();
		return;
	}
	else if( iAdress == 0xFF80 )
	{
		//TEMP : To avoid VRAM flush in Tetris
		m_aMemory.at( iAdress ) = 0;
		return;
	}

	m_aMemory.at( iAdress ) = iValue;
}

uint8_t& CPU::GetRefAtMemory( const uint16_t iAdress )
{
	return m_aMemory.at( iAdress );
}

void CPU::AddCPUInstruction( const uint8_t iIndex, const fct_opcode& pFct, std::array<uint8_t, 3> aValues, bool bExtent, const char* sMnemonic, ... )
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
	if( m_oCPUState == State::Pause )
		return;

	const int iMaxCycle = 70224;
	int cyclesThisUpdate = 0;

	while( cyclesThisUpdate < iMaxCycle )
	{
		if( m_oCPUState == State::Running || m_oCPUState == State::SkipToNext )
		{
			int iCycles = ExecuteNextOpcode();
			Timer::GetInstance()->UpdateTimers( iCycles );
			_UpdateLY( iCycles );
			PPU::GetInstance()->Update( iCycles );
			cyclesThisUpdate += iCycles; //TODO : if over 70224 cycles are lost
		}

		if( m_oCPUState == State::SkipToNext || m_aBreakpoints.find( m_iPC ) != m_aBreakpoints.end() )
		{
			m_oCPUState = State::Pause;
			return;
		}
	}
}

void CPU::_UpdateLY( const int iCycles )
{
	m_iLineCounter += iCycles;
	while( m_iLineCounter >= 456 )
	{
		m_iLineCounter -= 456;
		uint8_t iLY = ( ReadMemory( 0xFF44 ) + 1 ) % 154;
		WriteMemory( 0xFF44, iLY );
		if( iLY == 144 )
			RequestInterrupt( 0 );
	}
}

void CPU::LD_a16_SP()
{
	uint16_t iAdress = ( ReadMemory( m_iPC + 2 ) << 8 ) | ( ReadMemory( m_iPC + 1 ) );

	WriteMemory( iAdress, m_iSP & 0xFF );
	WriteMemory( iAdress + 1, m_iSP >> 8 );
}

void CPU::LD_HL_SPe()
{
	const uint8_t e8u = ReadMemory( m_iPC + 1 );
	int8_t e8 = static_cast< int8_t >( e8u );

	_SetFlag( ZERO_FLAG, 0 );
	_SetFlag( SUB_FLAG, 0 );
	_SetFlag( HALF_CARRY_FLAG,( ( m_iSP & 0xF ) + ( e8u & 0xF ) ) > 0xF );	//Set if overflow from bit 3
	_SetFlag( CARRY_FLAG,( m_iSP & 0xFF ) + e8u > 0xFF );					//Set if overflow from bit 7

	m_RegisterHL.reg = static_cast< uint16_t >( m_iSP + e8 );
}

void CPU::LD_SP_HL()
{
	m_iSP = m_RegisterHL.reg;
}

void CPU::LDH_n_A()
{
	uint8_t iValue = ReadMemory( m_iPC + 1 );
	WriteMemory( 0xFF00 + iValue,m_RegisterAF.hi );
}

void CPU::LDH_A_n()
{
	uint8_t iValue = ReadMemory( m_iPC + 1 );
	m_RegisterAF.hi = ReadMemory( 0xFF00 + iValue );
}

void CPU::LDH_C_A()
{
	WriteMemory( 0xFF00 + m_RegisterBC.lo, m_RegisterAF.hi );
}

void CPU::LDH_A_C()
{
	m_RegisterAF.hi = ReadMemory( 0xFF00 + m_RegisterBC.lo );
}

void CPU::LD_r16n16()
{
	uint16_t iValue = ( ReadMemory( m_iPC + 2 ) << 8 ) | ReadMemory( m_iPC + 1 );
	//std::cout << std::hex << std::uppercase << " ( 0X" << ( int )iValue << " )" << std::dec;

	_LDR16( _Get_Dest_R16(), iValue );
}

void CPU::LD_r8r8()
{
	_LDR8( _Get_Dest_R8(),_Get_Dest_R8( true ) );
}

void CPU::LD_HLI_A()
{
	WriteMemory( m_RegisterHL.reg,m_RegisterAF.hi );
	m_RegisterHL.reg++;
}

void CPU::LD_HLD_A()
{
	WriteMemory( m_RegisterHL.reg,m_RegisterAF.hi );
	m_RegisterHL.reg--;
}

void CPU::LD_r8n8()
{
	_LDR8( _Get_Dest_R8(),ReadMemory( m_iPC + 1 ) );
}

void CPU::LD_atR16A()
{
	WriteMemory( _Get_Mem_Address_R16(),m_RegisterAF.hi);
}

void CPU::LD_Aatr16()
{
	m_RegisterAF.hi = ReadMemory( _Get_Mem_Address_R16() );
}

void CPU::LD_n16A()
{
	uint16_t iAdress = ( ReadMemory( m_iPC + 2 ) << 8 ) | ReadMemory( m_iPC + 1 );
	WriteMemory( iAdress,m_RegisterAF.hi );
}

void CPU::LD_An16()
{
	uint16_t iAdress = ( ReadMemory( m_iPC + 2 ) << 8 ) | ReadMemory( m_iPC + 1 );
	m_RegisterAF.hi = ReadMemory( iAdress );
}

void CPU::CP()
{
	uint8_t& iData = _Get_Dest_R8( true );
	uint8_t iResult = m_RegisterAF.hi - iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG, 1 );
	_SetFlag( HALF_CARRY_FLAG,( m_RegisterAF.hi & 0xF ) < ( iData & 0xF ) );	//Set if Borrow from byte 4
	_SetFlag( CARRY_FLAG,iData > m_RegisterAF.hi );								//Set if Borrow
}

void CPU::CP_n8()
{
	const uint8_t iData = ReadMemory( m_iPC + 1 );
	uint8_t iResult = m_RegisterAF.hi - iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,1 );
	_SetFlag( HALF_CARRY_FLAG,( m_RegisterAF.hi & 0xF ) < ( iData & 0xF ) );	//Set if Borrow from byte 4
	_SetFlag( CARRY_FLAG,iData > m_RegisterAF.hi );								//Set if Borrow
}

void CPU::DAA()
{
	uint8_t iAValue = m_RegisterAF.hi;
	bool bHalfCarry = GetFlag( HALF_CARRY_FLAG );
	bool bCarry = GetFlag( CARRY_FLAG );
	uint8_t iOffset = 0;

	if( GetFlag( SUB_FLAG ) )
	{
		if( bHalfCarry )
			iOffset += 0x06;

		if( bCarry )
			iOffset += 0x60;

		iAValue -= iOffset;
	}
	else
	{
		if( bHalfCarry || ( ( iAValue & 0x0F ) > 0x09 ) )
			iOffset += 0x06;

		if( bCarry || ( iAValue > 0x99 ) )
		{
			iOffset += 0x60;
			_SetFlag( CARRY_FLAG,1 );
		}
		iAValue += iOffset;
	}

	_SetFlag( ZERO_FLAG,iAValue == 0 );
	_SetFlag( HALF_CARRY_FLAG,0 );

	m_RegisterAF.hi = iAValue;
}

void CPU::CPL()
{
	m_RegisterAF.hi = ~m_RegisterAF.hi;

	_SetFlag( SUB_FLAG,1 );
	_SetFlag( HALF_CARRY_FLAG,1 );
}

void CPU::SCF()
{
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,1 );
}

void CPU::CCF()
{
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG, !GetFlag( CARRY_FLAG ) );
}

void CPU::HALT()
{
	m_bHalted = true;
}

void CPU::OR()
{
	uint8_t& iData = _Get_Dest_R8( true );
	uint8_t iResult = m_RegisterAF.hi | iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,0 );

	m_RegisterAF.hi = iResult;
}

void CPU::OR_n8()
{
	const uint8_t iData = ReadMemory( m_iPC + 1 );
	uint8_t iResult = m_RegisterAF.hi | iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,0 );

	m_RegisterAF.hi = iResult;
}

void CPU::SUB()
{
	uint8_t& iData = _Get_Dest_R8( true );
	uint8_t iResult = m_RegisterAF.hi - iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,1 );
	_SetFlag( HALF_CARRY_FLAG,( m_RegisterAF.hi & 0xF ) < ( iData & 0xF ) );//Set if Borrow from byte 4
	_SetFlag( CARRY_FLAG,iData > m_RegisterAF.hi );

	m_RegisterAF.hi = iResult;
}

void CPU::SUB_n8()
{
	const uint8_t iData = ReadMemory( m_iPC + 1 );
	uint8_t iResult = m_RegisterAF.hi - iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,1 );
	_SetFlag( HALF_CARRY_FLAG,( m_RegisterAF.hi & 0xF ) < ( iData & 0xF ) );//Set if Borrow from byte 4
	_SetFlag( CARRY_FLAG,iData > m_RegisterAF.hi );

	m_RegisterAF.hi = iResult;
}

void CPU::SBC_n8()
{
	const uint8_t iData = ReadMemory( m_iPC + 1 );
	bool bCarryFlag = GetFlag( CARRY_FLAG );
	uint8_t iResult = m_RegisterAF.hi - ( iData + bCarryFlag );

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,1 );
	_SetFlag( HALF_CARRY_FLAG,( m_RegisterAF.hi & 0xF ) < ( iData & 0xF ) + bCarryFlag );//Set if Borrow from byte 4
	_SetFlag( CARRY_FLAG, bCarryFlag + iData > m_RegisterAF.hi );

	m_RegisterAF.hi = iResult;
}

void CPU::SBC_r8()
{
	uint8_t& iData = _Get_Dest_R8( true );
	bool bCarryFlag = GetFlag( CARRY_FLAG );
	uint8_t iResult = m_RegisterAF.hi - ( iData + bCarryFlag );

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,1 );
	_SetFlag( HALF_CARRY_FLAG,( m_RegisterAF.hi & 0xF ) < ( iData & 0xF ) + bCarryFlag );//Set if Borrow from byte 4
	_SetFlag( CARRY_FLAG, bCarryFlag + iData > m_RegisterAF.hi );

	m_RegisterAF.hi = iResult;
}

void CPU::AND()
{
	uint8_t& iData = _Get_Dest_R8( true );
	uint8_t iResult = m_RegisterAF.hi & iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,1 );
	_SetFlag( CARRY_FLAG,0 );

	m_RegisterAF.hi = iResult;
}

void CPU::AND_n8()
{
	const uint8_t iData = ReadMemory( m_iPC + 1 );
	uint8_t iResult = m_RegisterAF.hi & iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,1 );
	_SetFlag( CARRY_FLAG,0 );

	m_RegisterAF.hi = iResult;
}

void CPU::XOR()
{
	uint8_t& iData = _Get_Dest_R8( true );
	uint8_t iResult = m_RegisterAF.hi ^ iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,0 );

	m_RegisterAF.hi = iResult;
}

void CPU::XOR_n8()
{
	const uint8_t iData = ReadMemory( m_iPC + 1 );
	uint8_t iResult = m_RegisterAF.hi ^ iData;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,0 );

	m_RegisterAF.hi = iResult;
}

void CPU::DI()
{
	m_bIME = false;
	m_IME_Requested = -1;
}

void CPU::EI()
{
	m_IME_Requested = 1;
}

void CPU::RET_cc()
{
	const uint8_t iData = ReadMemory( m_iPC );
	uint8_t cc = ( iData & CC_MASK ) >> 3;

	if( _CheckCondition( cc ) )
		RET();
}

void CPU::RET()
{
	uint16_t iAdress;
	iAdress = m_aMemory[ m_iSP ]; //Lower byte
	++m_iSP;
	iAdress |= m_aMemory[ m_iSP ] << 8; //Upper byte
	++m_iSP;

	m_iPC = iAdress; //TODO : POP PC
	m_bJumped = true;
}

void CPU::RETI()
{
	RET();
	m_bIME = true;
	m_IME_Requested = -1;
}

void CPU::POP_r16()
{
	const uint8_t iData = ReadMemory( m_iPC );
	uint8_t r8 = ( iData & P_MASK ) >> 4;
	switch( r8 )
	{
		case 0: //BC
		{
			m_RegisterBC.lo = ReadMemory( m_iSP );
			++m_iSP;
			m_RegisterBC.hi = ReadMemory( m_iSP );
			++m_iSP;
			break;
		}
		case 1: //DE
		{
			m_RegisterDE.lo = ReadMemory( m_iSP );
			++m_iSP;
			m_RegisterDE.hi = ReadMemory( m_iSP );
			++m_iSP;
			break;
		}
		case 2: //HL
		{
			m_RegisterHL.lo = ReadMemory( m_iSP );
			++m_iSP;
			m_RegisterHL.hi = ReadMemory( m_iSP );
			++m_iSP;
			break;
		}
		case 3: //AF
		{
			m_RegisterAF.lo = ReadMemory( m_iSP ) & 0xF0;
			++m_iSP;
			m_RegisterAF.hi = ReadMemory( m_iSP );
			++m_iSP;
			break;
		}
	}
}

void CPU::PUSH_r16()
{
	const uint8_t iData = ReadMemory( m_iPC );
	uint8_t r8 = ( iData & P_MASK ) >> 4;
	switch( r8 )
	{
		case 0: //BC
		{
			--m_iSP;
			WriteMemory( m_iSP, m_RegisterBC.hi );
			--m_iSP;
			WriteMemory( m_iSP,	m_RegisterBC.lo );
			break;
		}
		case 1: //DE
		{
			--m_iSP;
			WriteMemory( m_iSP,m_RegisterDE.hi );
			--m_iSP;
			WriteMemory( m_iSP,m_RegisterDE.lo );
			break;
		}
		case 2: //HL
		{
			--m_iSP;
			WriteMemory( m_iSP,m_RegisterHL.hi );
			--m_iSP;
			WriteMemory( m_iSP,m_RegisterHL.lo );
			break;
		}
		case 3: //AF
		{
			--m_iSP;
			WriteMemory( m_iSP,m_RegisterAF.hi );
			--m_iSP;
			WriteMemory( m_iSP,m_RegisterAF.lo );
			break;
		}
	}
}

void CPU::JPn16()
{
	//Read next 2 bytes of memory
	uint16_t iAdress = ( ReadMemory( m_iPC + 2 ) << 8 ) | ReadMemory( m_iPC + 1 );
	m_iPC = iAdress;
	m_bJumped = true;
}

void CPU::JPccn16()
{
	const uint8_t iData = ReadMemory( m_iPC );
	uint8_t cc = ( iData & CC_MASK ) >> 3;
	if( _CheckCondition( cc ) )
		JPn16();
}

void CPU::JPHL()
{
	m_iPC = m_RegisterHL.reg;
	m_bJumped = true;
}

void CPU::CALL_n16()
{
	uint16_t iAdress = m_iPC + m_pCurrentInstruction->m_iLength;
	--m_iSP;
	m_aMemory[ m_iSP ] = iAdress >> 8; //Upper byte
	--m_iSP;
	m_aMemory[ m_iSP ] = iAdress &( ~0xFF00 ); //Lower byte
	JPn16();
}

void CPU::CALL_cc_n16()
{
	const uint8_t iData = ReadMemory( m_iPC );
	uint8_t cc = ( iData & CC_MASK ) >> 3;

	uint16_t iAdress = ( ReadMemory( m_iPC + 2 ) << 8 ) | ReadMemory( m_iPC + 1 );

	if( _CheckCondition( cc ) )
		CALL_n16();
}

void CPU::RST()
{
	const uint8_t iData = ReadMemory( m_iPC );
	uint8_t y = ( iData & Y_MASK ) >> 3;

	uint16_t iReturn = m_iPC + m_pCurrentInstruction->m_iLength;
	WriteMemory( --m_iSP,iReturn >> 8 );
	WriteMemory( --m_iSP,iReturn & 0xFF );

	m_iPC = y * 8;
	m_bJumped = true;
}

void CPU::RLCA()
{
	uint8_t& iR8 = m_RegisterAF.hi;
	uint8_t iFirstBit = ( iR8 >> 7 );
	uint8_t iResult = iR8 << 1;

	iResult |= iFirstBit;

	_SetFlag( ZERO_FLAG,0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iFirstBit );

	iR8 = iResult;
}

void CPU::RLA()
{
	uint8_t& iR8 = m_RegisterAF.hi;
	uint8_t iFirstBit = ( iR8 >> 7 );
	uint8_t iResult = iR8 << 1;

	iResult |= static_cast<uint8_t>( GetFlag( CARRY_FLAG ) );

	_SetFlag( ZERO_FLAG,0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iFirstBit );

	iR8 = iResult;
}

void CPU::RRCA()
{
	uint8_t& iR8 = m_RegisterAF.hi;
	uint8_t iLastBit = iR8 & 1;
	uint8_t iResult = iR8 >> 1;

	iResult |= ( iLastBit << 7 );

	_SetFlag( ZERO_FLAG,0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iLastBit );

	iR8 = iResult;
}

void CPU::RRA()
{
	uint8_t& iR8 = m_RegisterAF.hi;
	uint8_t iLastBit = iR8 & 1;
	uint8_t iResult = iR8 >> 1;

	iResult |= ( GetFlag( CARRY_FLAG ) << 7 );

	_SetFlag( ZERO_FLAG,0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iLastBit );

	iR8 = iResult;
}

void CPU::RRC()
{
	uint8_t& iR8 = _Get_Dest_R8( true );
	uint8_t iLastBit = iR8 & 1;
	uint8_t iResult = iR8 >> 1;

	iResult |= ( iLastBit << 7 );

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iLastBit );

	iR8 = iResult;
}

void CPU::RL()
{
	uint8_t& iR8 = _Get_Dest_R8( true );
	uint8_t iFirstBit = ( iR8 >> 7 );
	uint8_t iResult = iR8 << 1;

	uint8_t iFlag = GetFlag( CARRY_FLAG );
	iResult |= iFlag;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iFirstBit );

	iR8 = iResult;
}

void CPU::RR()
{
	uint8_t& iR8 = _Get_Dest_R8( true );
	uint8_t iLastBit = iR8 & 1;
	uint8_t iResult = iR8 >> 1;

	iResult |= ( GetFlag( CARRY_FLAG ) << 7 );

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iLastBit );

	iR8 = iResult;
}

void CPU::SWAP()
{
	uint8_t& iR8 = _Get_Dest_R8( true );

	uint8_t iResult = ( iR8 << 4 ) | ( iR8 >> 4 );

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,0 );

	iR8 = iResult;
}

void CPU::SLA()
{
	uint8_t& iR8 = _Get_Dest_R8( true );
	uint8_t iFirstBit = ( iR8 >> 7 );
	uint8_t iResult = iR8 << 1;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iFirstBit );

	iR8 = iResult;
}

void CPU::SRA()
{
	uint8_t& iR8 = _Get_Dest_R8( true );
	uint8_t iLastBit = iR8 & 1;
	uint8_t iResult = iR8 >> 1;

	iResult |= ( iR8 & 0x80 );

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iLastBit );

	iR8 = iResult;
}

void CPU::RLC()
{
	uint8_t& iR8 = _Get_Dest_R8( true );
	uint8_t iFirstBit = ( iR8 >> 7 );
	uint8_t iResult = iR8 << 1;

	iResult |= iFirstBit;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iFirstBit );

	iR8 = iResult;
}

void CPU::SRL()
{
	uint8_t& iR8 = _Get_Dest_R8( true );
	uint8_t iLastBit = iR8 & 1;
	uint8_t iResult = iR8 >> 1;

	_SetFlag( ZERO_FLAG,iResult == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,0 );
	_SetFlag( CARRY_FLAG,iLastBit );

	iR8 = iResult;
}

void CPU::BIT()
{
	uint8_t iBitIndex = 1 << _GetBitIndex();
	uint8_t iR8 = _Get_Dest_R8( true );

	_SetFlag( ZERO_FLAG,( iR8 & iBitIndex ) == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,1 );
}

void CPU::RES()
{
	uint8_t iBitIndex = 1 << _GetBitIndex();
	uint8_t& iR8 = _Get_Dest_R8( true );

	iR8 &= ~iBitIndex;
}

void CPU::SET()
{
	uint8_t iBitIndex = 1 << _GetBitIndex();
	uint8_t& iR8 = _Get_Dest_R8( true );

	iR8 |= iBitIndex;
}

void CPU::INC_r8()
{
	uint8_t& iData = _Get_Dest_R8();
	uint8_t iTemp = iData;
	iData += 1;

	_SetFlag( ZERO_FLAG,iData == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,( iTemp & 0xF ) == 0xF );//Set if overflow from bit 3.
}

void CPU::INC_r16()
{
	uint16_t& ir16 = _Get_Dest_R16();
	ir16 += 1;
}

void CPU::DEC_r8()
{
	uint8_t& iData = _Get_Dest_R8();
	uint8_t iTemp = iData;
	_LDR8( iData,iData - 1 );

	_SetFlag( ZERO_FLAG,iData == 0 );
	_SetFlag( SUB_FLAG,1 );
	_SetFlag( HALF_CARRY_FLAG,( iTemp & 0xF ) == 0 );//Set if borrow from bit 4.
}

void CPU::DEC_r16()
{
	uint16_t& ir16 = _Get_Dest_R16();
	_LDR16( ir16,ir16 - 1 );
}

void CPU::JR_n16()
{
	int8_t iOffset = static_cast< int8_t >( ReadMemory( m_iPC + 1 ) );
	m_iPC = m_iPC + iOffset;
}

void CPU::JR_cc_n16()
{
	const uint8_t iData = ReadMemory( m_iPC );
	uint8_t cc = ( iData & CC_MASK ) >> 3;
	int8_t iValue = static_cast< int8_t >( ReadMemory( m_iPC + 1 ) );

	if( _CheckCondition( cc ) )
		m_iPC += iValue;
}

void CPU::ADD_An8()
{
	const uint8_t iData = ReadMemory( m_iPC + 1 );
	int iResult = m_RegisterAF.hi + iData;

	_SetFlag( ZERO_FLAG,static_cast< uint8_t >( iResult ) == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,( ( m_RegisterAF.hi & 0xF ) + ( iData & 0xF ) ) > 0xF );//Set if overflow from bit 3
	_SetFlag( CARRY_FLAG,iResult > 0xFF );//Set if overflow from bit 7

	m_RegisterAF.hi = iResult;
}

void CPU::ADD_Ar8()
{
	uint8_t& iData = _Get_Dest_R8( true );
	int iResult = m_RegisterAF.hi + iData;

	_SetFlag( ZERO_FLAG,static_cast< uint8_t >( iResult ) == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,( ( m_RegisterAF.hi & 0xF ) + ( iData & 0xF ) ) > 0xF );//Set if overflow from bit 3
	_SetFlag( CARRY_FLAG,iResult > 0xFF );//Set if overflow from bit 7

	m_RegisterAF.hi = iResult;
}

void CPU::ADD_HLr16()
{
	uint16_t& iData = _Get_Dest_R16();
	int iResult = m_RegisterHL.reg + iData;

	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,( ( m_RegisterHL.reg & 0xFFF ) + ( iData & 0xFFF ) ) > 0xFFF );//Set if overflow from bit 11
	_SetFlag( CARRY_FLAG,iResult > 0xFFFF );//Set if overflow from bit 15

	m_RegisterHL.reg = iResult;
}

void CPU::ADD_SPe8()
{
	const uint8_t e8u =  ReadMemory( m_iPC + 1 );
	int8_t e8 = static_cast< int8_t >( e8u );

	_SetFlag( ZERO_FLAG,0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,( ( m_iSP & 0xF ) + ( e8u & 0xF ) ) > 0xF );//Set if overflow from bit 7
	_SetFlag( CARRY_FLAG,( m_iSP & 0xFF ) + e8u > 0xFF );

	m_iSP = static_cast< uint16_t >( m_iSP + e8 );
}

void CPU::ADC_An8()
{
	const uint8_t iData = ReadMemory( m_iPC + 1 );
	bool bCarryFlag = GetFlag( CARRY_FLAG );
	int iResult = m_RegisterAF.hi + iData + bCarryFlag;

	_SetFlag( ZERO_FLAG,static_cast< uint8_t >( iResult ) == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,( ( m_RegisterAF.hi & 0xF ) + ( iData & 0xF ) + bCarryFlag ) > 0xF );//Set if overflow from bit 3
	_SetFlag( CARRY_FLAG,iResult > 0xFF );//Set if overflow from bit 7

	m_RegisterAF.hi = iResult;
}

void CPU::ADC_Ar8()
{
	uint8_t& iData = _Get_Dest_R8( true );
	bool bCarryFlag = GetFlag( CARRY_FLAG );
	int iResult = m_RegisterAF.hi + iData + bCarryFlag;

	_SetFlag( ZERO_FLAG,static_cast< uint8_t >( iResult ) == 0 );
	_SetFlag( SUB_FLAG,0 );
	_SetFlag( HALF_CARRY_FLAG,( ( m_RegisterAF.hi & 0xF ) + ( iData & 0xF ) + bCarryFlag ) > 0xF );//Set if overflow from bit 3
	_SetFlag( CARRY_FLAG,iResult > 0xFF );//Set if overflow from bit 7

	m_RegisterAF.hi = iResult;
}

uint8_t& CPU::_Get_Dest_R8( bool s8 /*= false */)
{
	const uint8_t iData = ReadMemory( m_iPC );
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

uint16_t& CPU::_Get_Dest_R16()
{
	uint8_t r8 = ( ReadMemory( m_iPC ) & P_MASK ) >> 4;
	switch( r8 )
	{
		case 0: return m_RegisterBC.reg;							break;
		case 1: return m_RegisterDE.reg;							break;
		case 2: return m_RegisterHL.reg;							break;
		case 3: return m_iSP;										break;
	}
	throw std::runtime_error( "ERROR_GETTING_DESTR16" );
}

uint16_t CPU::_Get_Mem_Address_R16()
{
	uint8_t r8 = ( ReadMemory( m_iPC ) & P_MASK ) >> 4;
	switch( r8 )
	{
		case 0: return m_RegisterBC.reg;							break;
		case 1: return m_RegisterDE.reg;							break;
		case 2: return m_RegisterHL.reg++;							break;
		case 3: return m_RegisterHL.reg--;							break;
	}
	throw std::runtime_error( "ERROR_GETTING_DEST_MEMR16" );
}

uint8_t CPU::_GetBitIndex()
{
	return ( ReadMemory( m_iPC ) & Y_MASK ) >> 3;
}

bool CPU::_CheckCondition( uint8_t cc )
{
	switch( cc )
	{
		case 0: return !GetFlag( ZERO_FLAG );
		case 1: return GetFlag( ZERO_FLAG );
		case 2: return !GetFlag( CARRY_FLAG );
		case 3: return GetFlag( CARRY_FLAG );
	}

	throw std::runtime_error( "ERROR_GETTING_CONDITION" );
}

void CPU::_LDR8( uint8_t& iDst,uint8_t iData )
{
	iDst = iData;
}

void CPU::_LDR16( uint16_t& iDst,uint16_t iData )
{
	iDst = iData;
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
				case 0: CPU::AddCPUInstruction( i,{ &CPU::NOP },{ 1,4 },false,"NOP" ); break;
				case 1: CPU::AddCPUInstruction( i,{ &CPU::LD_a16_SP },{ 3,20 },false,"LD a16, SP" ); break;
				case 2: CPU::AddCPUInstruction( i,{ &CPU::STOP },{ 2,4 },false,"STOP" ); break;
				case 3: CPU::AddCPUInstruction( i,{ &CPU::JR_n16 },{ 2,12 },false,"JR n16" ); break;
				case 4:
				case 5:
				case 6:
				case 7:
					CPU::AddCPUInstruction( i,{ &CPU::JR_cc_n16 },{ 2,12,8 },false,"JR %s s8",cc[ y - 4 ].data() ); break;
				}
				break;
			}
			case 1:
			{
				switch( q )
				{
				case 0: CPU::AddCPUInstruction( i,{ &CPU::LD_r16n16 },{ 3,12 },false,"LD %s, d16",rp[ p ].data() ); break;
				case 1: CPU::AddCPUInstruction( i,{ &CPU::ADD_HLr16 },{ 1,8 },false,"ADD HL, %s",rp[ p ].data() ); break;
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
					case 0: CPU::AddCPUInstruction( i,{ &CPU::LD_atR16A },{ 1,8 },false,"LD BC, A" ); break;
					case 1: CPU::AddCPUInstruction( i,{ &CPU::LD_atR16A },{ 1,8 },false,"LD DE, A" ); break;
					case 2: CPU::AddCPUInstruction( i,{ &CPU::LD_HLI_A },{ 1,8 },false,"LD [HL+], A" ); break;
					case 3: CPU::AddCPUInstruction( i,{ &CPU::LD_HLD_A },{ 1,8 },false,"LD [HL-], A" ); break;
					}
					break;
				}
				case 1:
				{
					switch( p )
					{
					case 0: CPU::AddCPUInstruction( i,{ &CPU::LD_Aatr16 },{ 1,8 },false,"LD A, BC" ); break;
					case 1: CPU::AddCPUInstruction( i,{ &CPU::LD_Aatr16 },{ 1,8 },false,"LD A, DE" ); break;
					case 2: CPU::AddCPUInstruction( i,{ &CPU::LD_Aatr16 },{ 1,8 },false,"LD A, [HL+]" ); break;
					case 3: CPU::AddCPUInstruction( i,{ &CPU::LD_Aatr16 },{ 1,8 },false,"LD A, [HL-]" ); break;
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
				case 0: CPU::AddCPUInstruction( i,{ &CPU::INC_r16 },{ 1,8 },false,"INC %s",rp[ p ].data() ); break;
				case 1: CPU::AddCPUInstruction( i,{ &CPU::DEC_r16 },{ 1,8 },false,"DEC, %s",rp[ p ].data() ); break;
				}
				break;
			}
			case 4:
			{
				if( y == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::INC_r8 },{ 1,12 },false,"INC HL" );
				else
					CPU::AddCPUInstruction( i,{ &CPU::INC_r8 },{ 1,4 },false,"INC %c",r[ y ] );
				break;
			}
			case 5:
			{
				if( y == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::DEC_r8 },{ 1,12 },false,"DEC HL" );
				else
					CPU::AddCPUInstruction( i,{ &CPU::DEC_r8 },{ 1,4 },false,"DEC %c",r[ y ] );
				break;
			}
			case 6:
			{
				if( y == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::LD_r8n8 },{ 2,12 },false,"LD HL, d8" );
				else
					CPU::AddCPUInstruction( i,{ &CPU::LD_r8n8 },{ 2,8 },false,"LD %c, d8",r[ y ] );
				break;
			}
			case 7:
			{
				switch( y )
				{
				case 0: CPU::AddCPUInstruction( i,{ &CPU::RLCA },{ 1,4 },false,"RLCA" ); break;
				case 1: CPU::AddCPUInstruction( i,{ &CPU::RRCA },{ 1,4 },false,"RRCA" ); break;
				case 2: CPU::AddCPUInstruction( i,{ &CPU::RLA },{ 1,4 },false,"RLA" ); break;
				case 3: CPU::AddCPUInstruction( i,{ &CPU::RRA },{ 1,4 },false,"RRA" ); break;
				case 4: CPU::AddCPUInstruction( i,{ &CPU::DAA },{ 1,4 },false,"DAA" ); break;
				case 5: CPU::AddCPUInstruction( i,{ &CPU::CPL },{ 1,4 },false,"CPL" ); break;
				case 6: CPU::AddCPUInstruction( i,{ &CPU::SCF },{ 1,4 },false,"SCF" ); break;
				case 7: CPU::AddCPUInstruction( i,{ &CPU::CCF },{ 1,4 },false,"CCF" ); break;
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
				CPU::AddCPUInstruction( i,{ &CPU::HALT },{ 1,4 },false,"HALT" );
			}
			else
			{
				if( y == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::LD_r8r8 },{ 1,8 },false,"LD HL, %c",r[ z ] );
				else if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::LD_r8r8 },{ 1,8 },false,"LD %c, HL",r[ y ] );
				else
					CPU::AddCPUInstruction( i,{ &CPU::LD_r8r8 },{ 1,4 },false,"LD %c, %c",r[ y ],r[ z ] );
			}
			break;
		}
		case 2:
		{
			switch( y )
			{
			case 0:
			{
				if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::ADD_Ar8 },{ 1,8 },false,"%s HL",alu[ y ].data(),r[ z ] );
				else
					CPU::AddCPUInstruction( i,{ &CPU::ADD_Ar8 },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
				break;
			}

			case 1:
			{
				if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::ADC_Ar8 },{ 1,8 },false,"%s HL",alu[ y ].data(),r[ z ] );
				else
					CPU::AddCPUInstruction( i,{ &CPU::ADC_Ar8 },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
				break;
			}

			case 2:
			{
				if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::SUB },{ 1,8 },false,"%s HL",alu[ y ].data(),r[ z ] );
				else
				{
					if( z == 7 )
						CPU::AddCPUInstruction( i,{ &CPU::SUB },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
					else
						CPU::AddCPUInstruction( i,{ &CPU::SUB },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
				}
				break;
			}

			case 3:
			{
				if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::SBC_r8 },{ 1,8 },false,"%s HL",alu[ y ].data(),r[ z ] );
				else
				{
					if( z == 7 )
						CPU::AddCPUInstruction( i,{ &CPU::SBC_r8 },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
					else
						CPU::AddCPUInstruction( i,{ &CPU::SBC_r8 },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
				}
				break;
			}

			case 4:
			{
				if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::AND },{ 1,8 },false,"%s HL",alu[ y ].data(),r[ z ] );
				else
					CPU::AddCPUInstruction( i,{ &CPU::AND },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
				break;
			}

			case 5:
			{
				if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::XOR },{ 1,8 },false,"%s HL",alu[ y ].data(),r[ z ] );
				else
				{
					if( z == 7 )
						CPU::AddCPUInstruction( i,{ &CPU::XOR },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
					else
						CPU::AddCPUInstruction( i,{ &CPU::XOR },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
				}
				break;
			}

			case 6:
			{
				if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::OR },{ 1,8 },false,"%s HL",alu[ y ].data(),r[ z ] );
				else
					CPU::AddCPUInstruction( i,{ &CPU::OR },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
				break;
			}

			case 7:
			{
				if( z == 6 )
					CPU::AddCPUInstruction( i,{ &CPU::CP },{ 1,8 },false,"%s HL",alu[ y ].data(),r[ z ] );
				else
				{
					if( z == 7 )
						CPU::AddCPUInstruction( i,{ &CPU::CP },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
					else
						CPU::AddCPUInstruction( i,{ &CPU::CP },{ 1,4 },false,"%s %c",alu[ y ].data(),r[ z ] );
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
						CPU::AddCPUInstruction( i,{ &CPU::RET_cc },{ 1,20,8 },false,"RET %s",cc[ y ].data() ); break;
				case 4: CPU::AddCPUInstruction( i,{ &CPU::LDH_n_A },{ 2,12 },false,"LD (0xFF00+n), A" ); break;
				case 5: CPU::AddCPUInstruction( i,{ &CPU::ADD_SPe8 },{ 2,16 },false,"ADD SP, s8" ); break;
				case 6: CPU::AddCPUInstruction( i,{ &CPU::LDH_A_n },{ 2,8 },false,"LD A, (0xFF00+n)" ); break;
				case 7: CPU::AddCPUInstruction( i,{ &CPU::LD_HL_SPe },{ 2,12 },false,"LD HL, SP+s8" ); break;
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
						CPU::AddCPUInstruction( i,{ &CPU::POP_r16 },{ 1,12 },false,"POP %s",rp2[ p ].data() );
					else
						CPU::AddCPUInstruction( i,{ &CPU::POP_r16 },{ 1,12 },false,"POP %s",rp2[ p ].data() );
					break;
				}
				case 1:
				{
					switch( p )
					{
					case 0: CPU::AddCPUInstruction( i,{ &CPU::RET },{ 1,16 },false,"RET" ); break;
					case 1: CPU::AddCPUInstruction( i,{ &CPU::RETI },{ 1,16 },false,"RETI" ); break;
					case 2: CPU::AddCPUInstruction( i,{ &CPU::JPHL },{ 1,4 },false,"JP HL" ); break;
					case 3: CPU::AddCPUInstruction( i,{ &CPU::LD_SP_HL },{ 1,8 },false,"LD SP, HL" ); break;
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
							CPU::AddCPUInstruction( i,{ &CPU::JPccn16 },{ 3,16,12 },false,"JP %s a16",cc[ y ].data() ); break;
					case 4: CPU::AddCPUInstruction( i,{ &CPU::LDH_C_A },{ 1,8 },false,"LD (0xFF00+C), A",cc[ y ].data() ); break;
					case 5: CPU::AddCPUInstruction( i,{ &CPU::LD_n16A },{ 3,16 },false,"LD a16, A" ); break;
					case 6: CPU::AddCPUInstruction( i,{ &CPU::LDH_A_C },{ 1,8 },false,"LD A, (0xFF00+C)" ); break;
					case 7: CPU::AddCPUInstruction( i,{ &CPU::LD_An16 },{ 3,16 },false,"LD A, a16" ); break;
				}
				break;
			}
			case 3:
			{
				switch( y )
				{
				case 0: CPU::AddCPUInstruction( i,{ &CPU::JPn16 },{ 3,16 },false,"JP a16" ); break;
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
							if( z == 6 )
							{
								format = "%s (HL)";
								iDuration = 16;
							}
							switch( y )
							{
							case 0: CPU::AddCPUInstruction( k,{ &CPU::RLC },{ 2, iDuration },true,format,rot[ y ].data(),r[ z ] ); break;
							case 1: CPU::AddCPUInstruction( k,{ &CPU::RRC },{ 2, iDuration },true,format,rot[ y ].data(),r[ z ] ); break;
							case 2: CPU::AddCPUInstruction( k,{ &CPU::RL },{ 2, iDuration },true,format,rot[ y ].data(),r[ z ] ); break;
							case 3: CPU::AddCPUInstruction( k,{ &CPU::RR },{ 2, iDuration },true,format,rot[ y ].data(),r[ z ] ); break;
							case 4: CPU::AddCPUInstruction( k,{ &CPU::SLA },{ 2, iDuration },true,format,rot[ y ].data(),r[ z ] ); break;
							case 5: CPU::AddCPUInstruction( k,{ &CPU::SRA },{ 2, iDuration },true,format,rot[ y ].data(),r[ z ] ); break;
							case 6: CPU::AddCPUInstruction( k,{ &CPU::SWAP },{ 2, iDuration },true,format,rot[ y ].data(),r[ z ] ); break;
							case 7: CPU::AddCPUInstruction( k,{ &CPU::SRL },{ 2, iDuration },true,format,rot[ y ].data(),r[ z ] ); break;
							}
							break;
						}
						case 1:
						{
							if( z == 6 )
								CPU::AddCPUInstruction( k,{ &CPU::BIT },{ 2,12 },true,"BIT %i, HL",y );
							else
								CPU::AddCPUInstruction( k,{ &CPU::BIT },{ 2,8 },true,"BIT %i, %c",y,r[ z ] );
							break;
						}
						case 2:
						{
							if( z == 6 )
								CPU::AddCPUInstruction( k,{ &CPU::RES },{ 2,16 },true,"RES %i, HL",y );
							else
								CPU::AddCPUInstruction( k,{ &CPU::RES },{ 2,8 },true,"RES %i, %c",y,r[ z ] );
							break;
						}
						case 3:
						{
							if( z == 6 )
								CPU::AddCPUInstruction( k,{ &CPU::SET },{ 2,16 },true,"SET %i, HL",y );
							else
								CPU::AddCPUInstruction( k,{ &CPU::SET },{ 2,8 },true,"SET %i, %c",y,r[ z ] );
						}
						}
					}
					break;
				}
				case 6: CPU::AddCPUInstruction( i,{ &CPU::DI },{ 1,4 },false,"DI" ); break;
				case 7: CPU::AddCPUInstruction( i,{ &CPU::EI },{ 1,4 },false,"EI" ); break;
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
					CPU::AddCPUInstruction( i,{ &CPU::CALL_cc_n16 },{ 3,24,12 },false,"CALL %s, a16",cc[ y ].data() ); break;
				}
				break;
			}
			case 5:
			{
				if( q == 0 )
					CPU::AddCPUInstruction( i,{ &CPU::PUSH_r16 },{ 1,16 },false,"PUSH %s",rp2[ p ].data() );
				else if( q == 1 && p == 0 )
					CPU::AddCPUInstruction( i,{ &CPU::CALL_n16 },{ 3,24 },false,"CALL a16" );
				break;
			}
			case 6:
			{
				switch( y )
				{
				case 0: CPU::AddCPUInstruction( i,{ &CPU::ADD_An8 },{ 2,8 },false,"%s d8",alu[ y ].data() ); break;
				case 1: CPU::AddCPUInstruction( i,{ &CPU::ADC_An8 },{ 2,8 },false,"%s d8",alu[ y ].data() ); break;
				case 2: CPU::AddCPUInstruction( i,{ &CPU::SUB_n8 },{ 2,8 },false,"%s d8",alu[ y ].data() ); break;
				case 3: CPU::AddCPUInstruction( i,{ &CPU::SBC_n8 },{ 2,8 },false,"%s d8",alu[ y ].data() ); break;
				case 4: CPU::AddCPUInstruction( i,{ &CPU::AND_n8 },{ 2,8 },false,"%s d8",alu[ y ].data() ); break;
				case 5: CPU::AddCPUInstruction( i,{ &CPU::XOR_n8 },{ 2,8 },false,"%s d8",alu[ y ].data() ); break;
				case 6: CPU::AddCPUInstruction( i,{ &CPU::OR_n8 },{ 2,8 },false,"%s d8",alu[ y ].data() ); break;
				case 7: CPU::AddCPUInstruction( i,{ &CPU::CP_n8 },{ 2,8 },false,"%s d8",alu[ y ].data() ); break;
				}
				break;
			}
			case 7: CPU::AddCPUInstruction( i,{ &CPU::RST },{ 1,16 },false,"RST %02X",y * 8 ); break;
			}
			break;
		}
		default: std::cout << "Case not handled" << std::endl; break;
		}
	}
}