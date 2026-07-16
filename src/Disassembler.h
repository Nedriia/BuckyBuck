//
// Created by arthu on 09/07/2026.
//
#ifndef BUCKYBUCK_DISASSEMBLER_H
#define BUCKYBUCK_DISASSEMBLER_H

#include <cstdint>
#include <array>
#include "CPU.h"

enum CPU_FLAGS
{
	Z = 1 << 0,
	N = 1 << 1,
	H = 1 << 2,
	C = 1 << 3,
};

class Disassembler
{
public:
	Disassembler();
	~Disassembler();
	typedef void ( CPU::* fct_opcode )( );

	static void AddCPUInstruction( const uint8_t iIndex, const fct_opcode& pFct, uint8_t iFlags, uint8_t iFlagSet1, uint8_t iFlagReset0, std::array<uint8_t,3> aValues, bool bExtent, const char* sMnemonic, ... );
	static void Disassemble_ROM( const char* sRomPath );
	static std::string Format( const char* sFormat, ... );
	static void DecryptCartridge(std::fstream& file);
	static void DecryptIORange(std::fstream& file);
	void Init();


	typedef struct
	{
		char		m_sMnemonic[32];
		fct_opcode	m_pFct;
		uint8_t		m_aFlags; //Flags Affected
		uint8_t		m_aFlagSet_1;
		uint8_t		m_aFlagReset_0;
		uint8_t		m_iLength;
		uint8_t		m_iDuration; //T-state
		uint8_t		m_iConditionalDuration;
	} CPU_Instructions;
	static void _WriteInstruction( std::fstream& file, const uint16_t iAdress, const char* sComment, uint8_t* iLengthIncrease = nullptr );

//private:
	void _FillOpcodesTables();


	static CPU_Instructions* m_aOpcodesTable[256];
	static CPU_Instructions* m_aExtendOpcodesTable[256];
	static CPU m_oCPU;
};


#endif //BUCKYBUCK_DISASSEMBLER_H
