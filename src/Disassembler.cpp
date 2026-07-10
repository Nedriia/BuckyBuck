//
// Created by arthu on 09/07/2026.
//

#include <iostream>
#include "Disassembler.h"

static constexpr uint8_t X_MASK = 0b11000000;
static constexpr uint8_t Y_MASK = 0b00111000;
static constexpr uint8_t P_MASK = 0b00110000;
static constexpr uint8_t Q_MASK = 0b00001000;
static constexpr uint8_t Z_MASK = 0b00000111;

static constexpr uint32_t r[8] = { 'B','C','D','E','H','L',' ','A' };
static constexpr std::string_view rp[4] = { "BC","DE","(HL)","SP" };
static constexpr std::string_view rp2[4] = { "BC","DE","(HL)","AF" };
static constexpr std::string_view cc[4] = { "NZ","Z","NC","C" };
static constexpr std::string_view alu[8]= { "ADD A,","ADC A,","SUB","SBC A,","AND","XOR","OR","CP" };
static constexpr std::string_view rot[8]= { "RLC","RRC","RL","RR","SLA","SRA","SWAP","SRL" };

Disassembler::Disassembler()
	: m_aOpcodesTable{ 0 }
	, m_aExtendeOpcodesTable{ 0 }
{
	Init();
}

void Disassembler::Init()
{
	_FillOpcodesTables();
}

void Disassembler::_FillOpcodesTables()
{
	for ( int i = 0; i < 256; ++i )
	{
		uint8_t x = ( i & X_MASK ) >> 6;
		uint8_t y = ( i & Y_MASK ) >> 3;
		uint8_t p = ( i & P_MASK ) >> 4;
		uint8_t q = ( i & Q_MASK ) >> 3;
		uint8_t z = ( i & Z_MASK );

		switch ( x )
		{
			case 0:
			{
				switch ( z )
				{
					case 0:
					{
						switch ( y )
						{
							case 0:
							{
								std::cout << "NOP" << std::endl;
								break;
							}
							case 1:
							{
								std::cout << "LD (a16), SP" << std::endl;
								break;
							}
							case 2:
							{
								std::cout << "STOP" << std::endl;
								break;
							}
							case 3:
							{
								std::cout << "JR s8" << std::endl;
								break;
							}
							case 4:
							case 5:
							case 6:
							case 7:
							{
								std::cout << "JR " <<  cc[y-4] << ", s8" << std::endl;
								break;
							}
						}
						break;
					}
					case 1:
					{
						switch ( q )
						{
							case 0:
							{
								std::cout << "LD " << rp[p] << ", d16" << std::endl;
								break;
							}
							case 1:
							{
								std::cout << "ADD HL, " << rp[p] << std::endl;
								break;
							}
						}
						break;
					}
					case 2:
					{
						switch ( q )
						{
							case 0:
							{
								switch ( p )
								{
									case 0:
									{
										std::cout << "LD (BC), A" << std::endl;
										break;
									}
									case 1:
									{
										std::cout << "LD (DE), A" << std::endl;
										break;
									}
									case 2:
									{
										std::cout << "LD (HL+), A" << std::endl;
										break;
									}
									case 3:
									{
										std::cout << "LD (HL-), A" << std::endl;
										break;
									}
								}
								break;
							}
							case 1:
							{
								switch ( p )
								{
									case 0:
									{
										std::cout << "LD A, (BC)" << std::endl;
										break;
									}
									case 1:
									{
										std::cout << "LD A, (DE)" << std::endl;
										break;
									}
									case 2:
									{
										std::cout << "LD A, (HL+)" << std::endl;
										break;
									}
									case 3:
									{
										std::cout << "LD A, (HL-)" << std::endl;
										break;
									}
								}
								break;
							}
						}
						break;
					}
					case 3:
					{
						switch ( q )
						{
							case 0:
							{
								std::cout << "INC " << rp[p] << std::endl;
								break;
							}
							case 1:
							{
								std::cout << "DEC " << rp[p] << std::endl;
								break;
							}
						}
						break;
					}
					case 4:
					{
						if ( y == 6 )
							std::cout << "INC (HL)" << std::endl;
						else
							std::cout << "INC " << static_cast<char>( r[y] ) << std::endl;
						break;
					}
					case 5:
					{
						if ( y == 6 )
							std::cout <<  "DEC (HL)" << std::endl;
						else
							std::cout << "DEC " << static_cast<char>( r[y] ) << std::endl;
						break;
					}
					case 6:
					{
						if ( y == 6 )
							std::cout << "LD (HL), d8" << std::endl;
						else
							std::cout << "LD " << static_cast<char>( r[y] ) << ", d8" << std::endl;
						break;
					}
					case 7:
					{
						switch ( y )
						{
							case 0:
							{
								std::cout << "RLCA" << std::endl;
								break;
							}
							case 1:
							{
								std::cout << "RRCA" << std::endl;
								break;
							}
							case 2:
							{
								std::cout << "RLA" << std::endl;
								break;
							}
							case 3:
							{
								std::cout << "RRA" << std::endl;
								break;
							}
							case 4:
							{
								std::cout << "DAA" << std::endl;
								break;
							}
							case 5:
							{
								std::cout << "CPL" << std::endl;
								break;
							}
							case 6:
							{
								std::cout << "SCF" << std::endl;
								break;
							}
							case 7:
							{
								std::cout << "CCF" << std::endl;
								break;
							}
						}
						break;
					}
				}
				break;
			}
			case 1:
			{
				if ( z == 6 && y == 6)
				{
					std::cout << "HALT" << std::endl;
					break;
				}
				else if ( y == 6 )
				{
					std::cout << "LD (HL), " << static_cast<char>( r[z] ) << std::endl;
				}
				else if ( z == 6 )
				{
					std::cout << "LD " << static_cast<char>( r[y] ) << ", (HL)" << std::endl;
				}
				else
					std::cout << "LD " << static_cast<char>( r[y] ) << ", " << static_cast<char>( r[z] ) << std::endl;
				break;
			}
			case 2:
			{
				if ( z == 6 )
					std::cout << alu[y] << " (HL)" << std::endl;
				else
					std::cout <<  alu[y] << " " << static_cast<char>( r[z] ) << std::endl;
				break;
			}
			case 3:
			{
				switch ( z )
				{
					case 0:
					{
						switch ( y )
						{
							case 0:
							case 1:
							case 2:
							case 3:
							{
								std::cout << "RET " << cc[y] << std::endl;
								break;
							}
							case 4:
							{
								std::cout << "LD (a8), A" << std::endl;
								break;
							}
							case 5:
							{
								std::cout << "ADD SP, s8" << std::endl;
								break;
							}
							case 6:
							{
								std::cout << "LD A, (a8)" << std::endl;
								break;
							}
							case 7:
							{
								std::cout << "LD HL, SP+s8" << std::endl;
								break;
							}
						}
						break;
					}
					case 1:
					{
						switch ( q )
						{
							case 0:
							{
								std::cout << "POP " << rp2[p] << std::endl;
								break;
							}
							case 1:
							{
								switch ( p )
								{
									case 0:
									{
										std::cout << "RET" << std::endl;
										break;
									}
									case 1:
									{
										std::cout << "RETI" << std::endl;
										break;
									}
									case 2:
									{
										std::cout << "JP HL" << std::endl;
										break;
									}
									case 3:
									{
										std::cout << "LD SP, HL" << std::endl;
										break;
									}
								}
								break;
							}
						}
						break;
					}
					case 2:
					{
						switch ( y )
						{
							case 0:
							case 1:
							case 2:
							case 3:
							{
								std::cout << "JP " << cc[y] << ", a16" << std::endl;
								break;
							}
							case 4:
							{
								std::cout << "LD (C), A" << std::endl;
								break;
							}
							case 5:
							{
								std::cout << "LD (a16), A" << std::endl;
								break;
							}
							case 6:
							{
								std::cout << "LD A, (C)" << std::endl;
								break;
							}
							case 7:
							{
								std::cout << "LD A, (a16)" << std::endl;
								break;
							}
						}
						break;
					}
					case 3:
					{
						switch ( y )
						{
							case 0:
							{
								std::cout << "JP a16" << std::endl;
								break;
							}
							case 1:
							{
								std::cout << "(CB prefix)" << std::endl;
								for ( int k = 0; k < 256; ++k )
								{
									x = ( k & X_MASK ) >> 6;
									y = ( k & Y_MASK ) >> 3;
									z = ( k & Z_MASK );

									switch ( x )
									{
										case 0:
										{
											if ( z == 6 )
												std::cout << rot[y] << " (HL)" << std::endl;
											else
												std::cout << rot[y] << " " << static_cast<char>( r[z] ) << std::endl;
											break;
										}
										case 1:
										{
											if ( z == 6 )
												std::cout << "BIT " << static_cast<int>( y ) << ", (HL)" << std::endl;
											else
												std::cout << "BIT " << static_cast<int>( y ) << ", " << static_cast<char>( r[z] ) << std::endl;
											break;
										}
										case 2:
										{
											if ( z == 6 )
												std::cout << "RES " << static_cast<int>( y ) << ", (HL)" << std::endl;
											else
												std::cout << "RES " << static_cast<int>( y ) << ", " << static_cast<char>( r[z] ) << std::endl;
											break;
										}
										case 3:
										{
											if ( z == 6 )
												std::cout << "SET " << static_cast<int>( y ) << ", (HL)" << std::endl;
											else
												std::cout << "SET " << static_cast<int>( y ) << ", "  << static_cast<char>( r[z] ) << std::endl;
											break;
										}
									}
								}
								break;
							}
							case 6:
							{
								std::cout << "DI" << std::endl;
								break;
							}
							case 7:
							{
								std::cout << "EI" << std::endl;
								break;
							}
						}
						break;
					}
					case 4:
					{
						switch ( y )
						{
							case 0:
							case 1:
							case 2:
							case 3:
							{
								std::cout << "CALL " << cc[y] << ", a16" << std::endl;
								break;
							}
						}
						break;
					}
					case 5:
					{
						switch ( q )
						{
							case 0:
							{
								std::cout << "PUSH " << rp2[p] << std::endl;
								break;
							}
							case 1:
							{
								switch ( p )
								{
									case 0:
									{
										std::cout << "CALL a16" << std::endl;
										break;
									}
								}
								break;
							}
						}
						break;
					}
					case 6:
					{
						std::cout << alu[y] << " d8" << std::endl;
						break;
					}
					case 7:
					{
						std::cout << "RST " << static_cast< int >( y ) << std::endl;
						break;
					}
				}
				break;
			}
			default:
				std::cout << "Case not handled" << std::endl;
				break;
				//CPU_Instructions oTest; oTest.fct_opcode = { &Disassembler::NOP };
		}
	}
}



