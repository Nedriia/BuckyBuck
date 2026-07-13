//
// Created by arthu on 09/07/2026.
//

#include <iostream>
#include "Disassembler.h"

#include <stdio.h>
#include <stdarg.h>


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
Disassembler::CPU_Instructions* Disassembler::m_aExtendeOpcodesTable[ 256 ] = { nullptr };

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
		if( m_aExtendeOpcodesTable[ i ] != nullptr )
			delete m_aExtendeOpcodesTable[ i ];
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

	if( m_aExtendeOpcodesTable[ iIndex ] != nullptr && bExtent )
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
		m_aExtendeOpcodesTable[ iIndex ] = pInstruction;
	else
		m_aOpcodesTable[ iIndex ] = pInstruction;

	std::cout << pInstruction->m_sMnemonic << std::endl;
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
	std::cout << std::endl;
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
					case 0: AddCPUInstruction( i,{ &Disassembler::NOP },0,0,0,{1,4},false,"NOP" ); break;
					case 1: AddCPUInstruction( i,{ &Disassembler::LD }, 0,0,0,{3,20},false,"LD (a16), SP" ); break;
					case 2: AddCPUInstruction( i,{ &Disassembler::STOP },0,0,0,{2,4},false,"STOP" ); break;
					case 3: AddCPUInstruction( i,{ &Disassembler::JR }, 0,0,0,{2,12},false,"JR s8" ); break;
					case 4:
					case 5:
					case 6:
					case 7:
						AddCPUInstruction( i,{ &Disassembler::JR },0,0,0,{2,12,8},false,"JR %s s8",cc[ y-4 ].data() ); break;
				}
				break;
			}
			case 1:
			{
				switch( q )
				{
					case 0: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{3,12},false,"LD %s, d16",rp[ p ].data() ); break;
					case 1: AddCPUInstruction( i,{ &Disassembler::ADD },static_cast< uint8_t >( N | H | C ), (0), (N),{1,8},false,"ADD HL, %s",rp[ p ].data() ); break;
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
						case 0: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD (BC), A" ); break;
						case 1: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD (DE), A" ); break;
						case 2: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD (HL+), A" ); break;
						case 3: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD (HL-), A" ); break;
					}
					break;
				}
				case 1:
				{
					switch( p )
					{
						case 0: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD A, (BC)" ); break;
						case 1: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD A, (DE)" ); break;
						case 2: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD A, (HL+)" ); break;
						case 3: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD A, (HL-)" ); break;
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
				case 0: AddCPUInstruction( i,{ &Disassembler::INC },0,0,0,{1,8},false,"INC, %s",rp[ p ].data() ); break;
				case 1: AddCPUInstruction( i,{ &Disassembler::DEC },0,0,0,{1,8},false,"DEC, %s",rp[ p ].data() ); break;
				}
				break;
			}
			case 4:
			{
				if( y == 6 )
					AddCPUInstruction( i,{ &Disassembler::INC },( Z | N | H ),0, ( N ),{1,12},false,"INC (HL)" );
				else
					AddCPUInstruction( i,{ &Disassembler::INC },( Z | N | H ),0, ( N ),{1,4},false,"INC, %c",r[ y ] );
				break;
			}
			case 5:
			{
				if( y == 6 )
					AddCPUInstruction( i,{ &Disassembler::DEC },(Z | N | H),( N ), 0,{1,12},false,"DEC (HL)" );
				else
					AddCPUInstruction( i,{ &Disassembler::DEC },(Z | N | H),( N ), 0,{1,4},false,"DEC, %c",r[ y ] );
				break;
			}
			case 6:
			{
				if( y == 6 )
					AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{2,12},false,"LD (HL), d8" );
				else
					AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{2,8},false,"LD, %c, d8",r[ y ] );
				break;
			}
			case 7:
			{
				switch( y )
				{
					case 0: AddCPUInstruction( i,{ &Disassembler::RLCA },( Z | N | H | C ), 0, ( Z | N | H ),{1,4},false,"RLCA" ); break;
					case 1: AddCPUInstruction( i,{ &Disassembler::RRCA },( Z | N | H | C ), 0, ( Z | N | H ),{1,4},false,"RRCA" ); break;
					case 2: AddCPUInstruction( i,{ &Disassembler::RLA }, ( Z | N | H | C ), 0, ( Z | N | H ),{1,4},false,"RLA" ); break;
					case 3: AddCPUInstruction( i,{ &Disassembler::RRA }, ( Z | N | H | C ), 0, ( Z | N | H ),{1,4},false,"RRA" ); break;
					case 4: AddCPUInstruction( i,{ &Disassembler::DAA }, ( Z     | H | C ), 0, ( H ),{1,4},false,"DAA" ); break;
					case 5: AddCPUInstruction( i,{ &Disassembler::CPL }, (	   N | H	),  ( N | H	), 0,{1,4},false,"CPL" ); break;
					case 6: AddCPUInstruction( i,{ &Disassembler::SCF }, (     N | H | C ),( C ), ( N | H	),{1,4},false,"SCF" ); break;
					case 7: AddCPUInstruction( i,{ &Disassembler::CCF }, (     N | H | C ),0, ( N | H ),{1,4},false,"CCF" ); break;
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
				AddCPUInstruction( i,{ &Disassembler::HALT },0,0,0,{1,4},false,"HALT" );
			}
			else
			{
				if( y == 6 )
					AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD (HL), %c",r[ z ] );
				else if( z == 6 )
					AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD %c, (HL)",r[ y ] );
				else
					AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,4},false,"LD %c, %c",r[ y ],r[ z ] );
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
						AddCPUInstruction( i,{ &Disassembler::ADD },( Z | N | H | C ),0, ( N ),{1,8},false,"%s (HL)", alu[ y ].data(),r[ z ] );
					else
						AddCPUInstruction( i,{ &Disassembler::ADD },( Z | N | H | C ),0,( N ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 1:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &Disassembler::ADC },( Z | N | H | C ),0, ( N ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
						AddCPUInstruction( i,{ &Disassembler::ADC },( Z | N | H | C ),0,( N ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 2:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &Disassembler::SUB },( Z | N | H | C ),( N ), 0,{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							AddCPUInstruction( i,{ &Disassembler::SUB },( Z | N | H | C ),( Z | N ),( H | C ),{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
						else
							AddCPUInstruction( i,{ &Disassembler::SUB },( Z | N | H | C ),( N ),0,{1,4},false,"%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 3:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &Disassembler::SBC },( Z | N | H | C ),( N ),0,{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							AddCPUInstruction( i,{ &Disassembler::SBC },( Z | N | H ), ( N ), 0,{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							AddCPUInstruction( i,{ &Disassembler::SBC },( Z | N | H | C ),( N ), 0,{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 4:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &Disassembler::AND },( Z | N | H | C ), ( H ),( N | C ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
						AddCPUInstruction( i,{ &Disassembler::AND },( Z | N | H | C ), ( H ),( N | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 5:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &Disassembler::XOR },( Z | N | H | C ), ( Z ),( N | H | C ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							AddCPUInstruction( i,{ &Disassembler::XOR },( Z | N | H | C ), ( Z ),( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							AddCPUInstruction( i,{ &Disassembler::XOR },( Z | N | H | C ), 0,( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					}
					break;
				}

				case 6:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &Disassembler::OR },( Z | N | H | C ), 0,( N | H | C ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
						AddCPUInstruction( i,{ &Disassembler::OR },( Z | N | H | C ), ( Z ),( N | H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
					break;
				}

				case 7:
				{
					if ( z == 6 )
						AddCPUInstruction( i,{ &Disassembler::CP },( Z | N | H | C ), 0,( N ),{1,8},false, "%s (HL)", alu[ y ].data(),r[ z ] );
					else
					{
						if ( z == 7 )
							AddCPUInstruction( i,{ &Disassembler::CP },( Z | N | H | C ), ( Z | N ),( H | C ),{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
						else
							AddCPUInstruction( i,{ &Disassembler::CP },( Z | N | H | C ), ( N ),0,{1,4},false, "%s %c", alu[ y ].data(),r[ z ] );
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
							AddCPUInstruction( i,{ &Disassembler::RET },0,0,0,{1,20,8},false,"RET %s",cc[ y ].data() ); break;
					case 4: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{2,12},false,"LD (a8), A" ); break;
					case 5: AddCPUInstruction( i,{ &Disassembler::ADD },( Z | N | H | C ),0,( Z | N ), {2,16},false,"ADD SP, s8" ); break;
					case 6: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{2,8},false,"LD A, (a8)" ); break;
					case 7: AddCPUInstruction( i,{ &Disassembler::LD }, ( Z | N | H | C ),0,( Z | N ),{2,12},false,"LD HL, SP+s8" ); break;
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
						AddCPUInstruction( i,{ &Disassembler::POP },( Z | N | H | C ),0,0,{1,12},false,"POP %s",rp2[ p ].data() );
					else
						AddCPUInstruction( i,{ &Disassembler::POP },0,0,0,{1,12},false,"POP %s",rp2[ p ].data() );
					break;
				}
				case 1:
				{
					switch( p )
					{
						case 0: AddCPUInstruction( i,{ &Disassembler::RET },0,0,0,{1,16},false,"RET" ); break;
						case 1: AddCPUInstruction( i,{ &Disassembler::RETI },0,0,0,{1,16},false,"RETI" ); break;
						case 2: AddCPUInstruction( i,{ &Disassembler::JP },0,0,0,{1,4},false,"JP HL" ); break;
						case 3: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD SP, HL" ); break;
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
							AddCPUInstruction( i,{ &Disassembler::JP },0,0,0,{3,16,12},false,"JP %s a16",cc[ y ].data() ); break;
					case 4: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD (C), A",cc[ y ].data() ); break;
					case 5: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{3,16},false,"LD a16, A" ); break;
					case 6: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{1,8},false,"LD A, (C)" ); break;
					case 7: AddCPUInstruction( i,{ &Disassembler::LD },0,0,0,{3,16},false,"JP A, a16" ); break;
				}
				break;
			}
			case 3:
			{
				switch( y )
				{
				case 0: AddCPUInstruction( i,{ &Disassembler::JP },0,0,0,{3,16},false,"JP a16" ); break;
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
								case 0: AddCPUInstruction( k,{ &Disassembler::RLC }, ( Z | N | H | C ), 0,( N | H ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 1: AddCPUInstruction( k,{ &Disassembler::RRC }, ( Z | N | H | C ), 0,( N | H ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 2: AddCPUInstruction( k,{ &Disassembler::RL },  ( Z | N | H | C ), 0,( N | H ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 3: AddCPUInstruction( k,{ &Disassembler::RR },  ( Z | N | H | C ), 0,( N | H ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 4: AddCPUInstruction( k,{ &Disassembler::SLA }, ( Z | N | H | C ), 0,( N | H ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 5: AddCPUInstruction( k,{ &Disassembler::SRA }, ( Z | N | H | C ), 0,( N | H ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 6: AddCPUInstruction( k,{ &Disassembler::SWAP },( Z | N | H | C ), 0,( N | H | C ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
								case 7: AddCPUInstruction( k,{ &Disassembler::SRL }, ( Z | N | H | C ), 0,( N | H ), { 2, iDuration },true, format,rot[ y ].data(), r[ z ] ); break;
							}
							break;
						}
						case 1:
						{
							if( z == 6 )
								AddCPUInstruction( k,{ &Disassembler::BIT },( Z | N | H ), ( H ),( N ),{2,12},true,"BIT %i, (HL)",y );
							else
								AddCPUInstruction( k,{ &Disassembler::BIT },( Z | N | H ),( H ),( N ),{2,8},true,"BIT %i, %c",y,r[ z ] );
							break;
						}
						case 2:
						{
							if( z == 6 )
								AddCPUInstruction( k,{ &Disassembler::RES },0,0,0,{2,16},true,"RES %i, (HL)",y );
							else
								AddCPUInstruction( k,{ &Disassembler::RES },0,0,0,{2,8},true,"RES %i, %c",y,r[ z ] );
							break;
						}
						case 3:
						{
							if( z == 6 )
								AddCPUInstruction( k,{ &Disassembler::SET },0,0,0,{2,16},true,"SET %i, (HL)",y );
							else
								AddCPUInstruction( k,{ &Disassembler::SET },0,0,0,{2,8},true,"SET %i, %c",y,r[ z ] );
						}
						}
					}
					break;
				}
				case 6: AddCPUInstruction( i,{ &Disassembler::DI },0,0,0,{1,4},false,"DI" ); break;
				case 7: AddCPUInstruction( i,{ &Disassembler::EI },0,0,0,{1,4},false,"EI" ); break;
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
						AddCPUInstruction( i,{ &Disassembler::PUSH },0,0,0,{3,24,12},false,"CALL %s, a16",cc[ y ].data() ); break;
				}
				break;
			}
			case 5:
			{
				if( q == 0 )
					AddCPUInstruction( i,{ &Disassembler::PUSH },0,0,0,{1,16},false,"PUSH %s",rp2[ p ].data() );
				else if( q == 1 && p == 0 )
					AddCPUInstruction( i,{ &Disassembler::CALL },0,0,0,{3,24},false,"CALL a16" );
				break;
			}
			case 6:
			{
				switch( y )
				{
					case 0: AddCPUInstruction( i,{ &Disassembler::ADD },( Z | N | H | C ), 0,( N ),{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 1: AddCPUInstruction( i,{ &Disassembler::ADC },( Z | N | H | C ), 0,( N ),{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 2: AddCPUInstruction( i,{ &Disassembler::SUB },( Z | N | H | C ), ( N ),0,{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 3: AddCPUInstruction( i,{ &Disassembler::SBC },( Z | N | H | C ), ( N ),0,{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 4: AddCPUInstruction( i,{ &Disassembler::AND },( Z | N | H | C ), ( H ), ( N | C ),{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 5: AddCPUInstruction( i,{ &Disassembler::XOR },( Z | N | H | C ), 0,( N | H | C ),{2,8},false,"%s d8",alu[ y ].data() ); break;
					case 6: AddCPUInstruction( i,{ &Disassembler::OR }, ( Z | N | H | C ), 0,( N | H | C ), {2,8},false,"%s d8",alu[ y ].data() ); break;
					case 7: AddCPUInstruction( i,{ &Disassembler::CP }, ( Z | N | H | C ), ( N ),0, {2,8},false,"%s d8",alu[ y ].data() ); break;
				}
				break;
			}
			case 7: AddCPUInstruction( i,{ &Disassembler::RST },0,0,0,{1,16},false,"RST %i",y ); break;
			}
			break;
		}
		default: std::cout << "Case not handled" << std::endl; break;
		}
	}
}