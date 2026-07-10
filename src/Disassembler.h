//
// Created by arthu on 09/07/2026.
//
#ifndef BUCKYBUCK_DISASSEMBLER_H
#define BUCKYBUCK_DISASSEMBLER_H

#include <cstdint>
#include <string>

class Disassembler
{
public:
	Disassembler();
	void Init();

private:
	void _FillOpcodesTables();

	typedef struct
	{
		std::string m_sMnemonic;
		uint8_t m_iDuration; //T-state
		void ( Disassembler::* fct_opcode )();

	} CPU_Instructions;

	CPU_Instructions m_aOpcodesTable[256];
	CPU_Instructions m_aExtendeOpcodesTable[256];

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
