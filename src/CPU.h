//
// Created by arthu on 16/07/2026.
//

#ifndef BUCKYBUCK_CPU_H
#define BUCKYBUCK_CPU_H
#include <cstdint>
#include <array>
#include <iostream>

enum CPU_FLAGS
{
	Z = 1 << 0,
	N = 1 << 1,
	H = 1 << 2,
	C = 1 << 3,
};

class CPU
{
	union Register
	{
		uint16_t reg;
		struct
		{
			uint8_t lo;
			uint8_t hi;
		};
	};

	public:
	CPU();
	~CPU();

	void			Init();
	int				LoadRom( const char* sROMPath );
	void			FetchDecode();
	void			EmulateCycle();
	void			DestroyInstance();

	uint8_t			GetDataAtAdress( const uint16_t iAdress ) const { return m_aMemory[iAdress]; }

	typedef void ( CPU::* fct_opcode )( );
	static void		AddCPUInstruction( const uint8_t iIndex, const fct_opcode& pFct, uint8_t iFlags, uint8_t iFlagSet1, uint8_t iFlagReset0, std::array<uint8_t,3> aValues, bool bExtent, const char* sMnemonic, ... );

	static CPU* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new CPU;
		return m_pSingleton;
	}

	private:

	void 			_FillOpcodesTables();
	void			_SetValueToRegisterR8( const uint8_t iIndexRegister, const uint8_t iValue );

	uint16_t m_iPC;
	uint16_t m_iSP;

	uint8_t m_iIR;	//Instruction Register
	uint8_t m_iIE;	//Interrupt Enable

	Register m_RegisterAF;
	Register m_RegisterBC;
	Register m_RegisterDE;
	Register m_RegisterHL;

	uint8_t m_aMemory[0x7FFF];

	void NOP();
	void LD()							{		std::cout << "NOT IMPLEMENTED"; };
	void LD_HLd16();
	void LD_r8r8();
	void LD_A_HLI();
	void LD_A_HLD();
	void LD_r8d8();
	void LD_r16A();
	void INC()							{		std::cout << "NOT IMPLEMENTED"; };
	void INC_r8();
	void DEC()							{		std::cout << "NOT IMPLEMENTED"; };
	void RLCA()							{		std::cout << "NOT IMPLEMENTED"; };
	void RRCA()							{		std::cout << "NOT IMPLEMENTED"; };
	void RLA()							{		std::cout << "NOT IMPLEMENTED"; };
	void RRA()							{		std::cout << "NOT IMPLEMENTED"; };
	void STOP()							{		std::cout << "NOT IMPLEMENTED"; };
	void JR()							{		std::cout << "NOT IMPLEMENTED"; };
	void ADD()							{		std::cout << "NOT IMPLEMENTED"; };
	void ADC()							{		std::cout << "NOT IMPLEMENTED"; };
	void SUB()							{		std::cout << "NOT IMPLEMENTED"; };
	void SBC()							{		std::cout << "NOT IMPLEMENTED"; };
	void AND()							{		std::cout << "NOT IMPLEMENTED"; };
	void XOR()							{		std::cout << "NOT IMPLEMENTED"; };
	void OR()							{		std::cout << "NOT IMPLEMENTED"; };
	void CP()							{		std::cout << "NOT IMPLEMENTED"; };
	void DAA()							{		std::cout << "NOT IMPLEMENTED"; };
	void CPL()							{		std::cout << "NOT IMPLEMENTED"; };
	void SCF()							{		std::cout << "NOT IMPLEMENTED"; };
	void CCF()							{		std::cout << "NOT IMPLEMENTED"; };
	void HALT()							{		std::cout << "NOT IMPLEMENTED"; };
	void DI()							{		std::cout << "NOT IMPLEMENTED"; };
	void EI()							{		std::cout << "NOT IMPLEMENTED"; };
	void RET()							{		std::cout << "NOT IMPLEMENTED"; };
	void RETI()							{		std::cout << "NOT IMPLEMENTED"; };
	void POP()							{		std::cout << "NOT IMPLEMENTED"; };
	void PUSH()							{		std::cout << "NOT IMPLEMENTED"; };
	void JP()							{		std::cout << "NOT IMPLEMENTED"; };
	void JPn16();
	void CALL()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void RST()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void RLC()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void RRC()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void RL()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void RR()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void SLA()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void SRA()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void SWAP()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void SRL()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void BIT()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void RES()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };
	void SET()							{		std::cout << "NOT IMPLEMENTED" << std::endl; };

	friend class Disassembler;

	static CPU* m_pSingleton;

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

	static CPU_Instructions* m_aOpcodesTable[256];
	static CPU_Instructions* m_aExtendOpcodesTable[256];
};


#endif //BUCKYBUCK_CPU_H
