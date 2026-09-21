//
// Created by arthu on 16/07/2026.
//

#ifndef BUCKYBUCK_CPU_H
#define BUCKYBUCK_CPU_H
#include <array>

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

	Register m_RegisterAF;
	Register m_RegisterBC;
	Register m_RegisterDE;
	Register m_RegisterHL;

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

	uint16_t m_iPC;
	uint16_t m_iSP;

	uint8_t m_iIR;	//Instruction Register
	uint8_t m_iIE;	//Interrupt Enable

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
