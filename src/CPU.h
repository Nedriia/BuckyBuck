//
// Created by arthu on 16/07/2026.
//

#ifndef BUCKYBUCK_CPU_H
#define BUCKYBUCK_CPU_H
#include <cstdint>
#include <array>
#include <iostream>
#include <vector>
#include <unordered_set>

enum State
{
	Pause,
	Running,
	SkipToNext,
	Reset
};

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

	//Control Flow
	bool			IsRunning() const { return m_oCPUState == State::Running; }

	void			Run()			{ m_oCPUState = State::Running; }
	void			Reset();
	void			Pause()			{ m_oCPUState = State::Pause; }
	void			SkipToNext()	{ m_oCPUState = State::SkipToNext; }
	void			MarkBreakpoint( const uint16_t iAdress );
	bool			IsBreakpoint( const uint16_t iAdress ) const;

	uint8_t							ReadMemory( const uint16_t iAdress ) const;
	void							WriteMemory( const uint16_t iAdress, const uint8_t iValue );
	uint8_t&						GetRefAtMemory( const uint16_t iAdress );

	uint16_t						GetPC() const { return m_iPC; }
	uint16_t						GetSP() const { return m_iSP; }

	typedef void ( CPU::* fct_opcode )( );
	static void		AddCPUInstruction( const uint8_t iIndex, const fct_opcode& pFct, uint8_t iFlags, uint8_t iFlagSet1, uint8_t iFlagReset0, std::array<uint8_t,3> aValues, bool bExtent, const char* sMnemonic, ... );

	static CPU* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new CPU;
		return m_pSingleton;
	}

	//TEMP
	Register m_RegisterAF;
	Register m_RegisterBC;
	Register m_RegisterDE;
	Register m_RegisterHL;

	bool	ZFlag;
	bool	NFlag;
	bool	HFlag;
	bool	CFlag;
	bool	IME;
	static const std::string& GetMemoryRegionLabel( const uint16_t iAdress );

private:

	void 			_FillOpcodesTables();
	void			_SimulateBootRom();
	void			_UpdateFlags();

	uint8_t&		_Get_Dest_R8( bool s8 = false );
	uint16_t&		_Get_Dest_R16();
	uint16_t		_Get_Mem_Address_R16();
	uint16_t&		_Get_Destr16Stk(){};
	uint8_t&		_GetBitIndex();
	bool			_CheckCondition( uint8_t cc );
	void			_LDR8( uint8_t& iDst, uint8_t iData );
	void			_LDR16( uint16_t& iDst, uint16_t iData );

	uint16_t m_iPC;
	uint16_t m_iSP;
	long long m_iCounter;

	uint8_t m_iIR;	//Instruction Register

	void NOP(){};
	void LD()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void LDH_n_A();
	void LDH_A_n();
	void LDH_A_C();
	void LDH_C_A();
	void LD_r16n16();
	void LD_r8r8();
	void LD_HLI_A();
	void LD_HLD_A();
	void LD_r8n8();
	void LD_atR16A();
	void LD_n16A();
	void LD_An16();
	void LD_Aatr16();
	void INC_r8();
	void INC_r16();
	void DEC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void DEC_r8();
	void RLCA();
	void RRCA();
	void RLA();
	void RRA();
	void STOP()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void JR()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void JR_n16();
	void JR_cc_n16();
	void ADD()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void ADD_An8();
	void ADC_An8();
	void ADC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SUB();
	void SUB_n8();
	void SBC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void AND();
	void AND_n8();
	void XOR();
	void XOR_n8();
	void OR();
	void OR_n8();
	void CP();
	void CP_n8();
	void DAA()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void CPL()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SCF()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void CCF()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void HALT()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void DI();
	void EI();
	void RET_cc();
	void RET();
	void RETI()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void POP()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void POP_r16();
	void PUSH()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void PUSH_r16();
	void JP()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void JPn16();
	void CALL_n16();
	void CALL_cc_n16();
	void RST()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RLC();
	void RRC();
	void RL();
	void RR();
	void SLA();
	void SRA();
	void SWAP();
	void SRL();
	void BIT();
	void RES();
	void SET();

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

	struct MemoryRegion
	{
		uint16_t m_iStartAdress = 0;
		uint16_t m_iEndAdress = 0;
		std::string m_sLabel;
	};
	static MemoryRegion m_aMemoryMap[12];

	State m_oCPUState;

	std::vector<uint8_t> m_aROM_Memory;
	std::vector<uint8_t> m_aMemory;

	CPU_Instructions* m_pCurrentInstruction;
	std::unordered_set< uint16_t > m_aBreakpoints;
};


#endif //BUCKYBUCK_CPU_H
