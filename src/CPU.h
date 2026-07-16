//
// Created by arthu on 16/07/2026.
//

#ifndef BUCKYBUCK_CPU_H
#define BUCKYBUCK_CPU_H
#include <cstdint>

class CPU
{
public:
	CPU();
	int LoadRom( const char* sROMPath );
	uint8_t GetDataAtAdress( const uint16_t iAdress ) const { return m_aMemory[iAdress]; }
private:
	uint8_t m_aMemory[0x7FFF];

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

	friend class Disassembler;
};


#endif //BUCKYBUCK_CPU_H
