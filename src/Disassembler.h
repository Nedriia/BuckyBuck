//
// Created by arthu on 09/07/2026.
//
#ifndef BUCKYBUCK_DISASSEMBLER_H
#define BUCKYBUCK_DISASSEMBLER_H

#include <cstdint>
#include <array>

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
	typedef void ( Disassembler::* fct_opcode )( );

	static void AddCPUInstruction( const uint8_t iIndex, const fct_opcode& pFct, uint8_t iFlags, uint8_t iFlagSet1, uint8_t iFlagReset0, std::array<uint8_t,3> aValues, bool bExtent, const char* sMnemonic, ... );

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
//private:
	void _FillOpcodesTables();


	static CPU_Instructions* m_aOpcodesTable[256];
	static CPU_Instructions* m_aExtendeOpcodesTable[256];

	void NOP() {};
	void LD() {};
	void INC() {};
	void DEC() {};
	void RLCA() {};
	void RRCA() {};
	void RLA() {};
	void RRA() {};
	void STOP() {};
	void JR() {};
	void ADD() {};
	void ADC() {};
	void SUB() {};
	void SBC() {};
	void AND() {};
	void XOR() {};
	void OR() {};
	void CP() {};
	void DAA() {};
	void CPL() {};
	void SCF() {};
	void CCF() {};
	void HALT() {};
	void DI() {};
	void EI() {};
	void RET() {};
	void RETI() {};
	void POP() {};
	void PUSH() {};
	void JP() {};
	void CALL() {};
	void RST() {};
	void RLC() {};
	void RRC() {};
	void RL() {};
	void RR() {};
	void SLA() {};
	void SRA() {};
	void SWAP() {};
	void SRL() {};
	void BIT() {};
	void RES() {};
	void SET() {};
};


#endif //BUCKYBUCK_DISASSEMBLER_H
