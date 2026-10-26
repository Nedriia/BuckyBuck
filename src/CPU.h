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
	static const std::string& GetMemoryRegionLabel( const uint16_t iAdress );

private:

	void 			_FillOpcodesTables();
	void			_SimulateBootRom();
	void			_UpdateFlags();

	uint8_t&		_Get_Dest_R8( bool s8 = false );
	uint16_t&		_Get_Dest_R16( bool bMem = false );
	uint16_t&		_Get_Destr16Stk(){};
	bool			_CheckCondition( uint8_t cc );
	void			_LDR8( uint8_t& iDst, uint8_t iData );
	void			_LDR16( uint16_t& iDst, uint16_t iData );

	uint16_t m_iPC;
	uint16_t m_iSP;

	uint8_t m_iIR;	//Instruction Register

	void NOP(){};
	void LD()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void LDH()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void LDH_C()						{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void LD_r16n16();
	void LD_r8r8();
	void LD_A_HLI();
	void LD_A_HLD();
	void LD_r8n8();
	void LD_atR16A();
	void INC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void INC_r8();
	void DEC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void DEC_r8();
	void RLCA()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RRCA()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RLA()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RRA()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void STOP()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void JR()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void JR_cc_n16();
	void ADD()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void ADC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SUB()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SBC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void AND()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void XOR()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void OR()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void CP()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void DAA()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void CPL()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SCF()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void CCF()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void HALT()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void DI()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void EI()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RET()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RETI()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void POP()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void PUSH()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void JP()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void JPn16();
	void CALL()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RST()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RLC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RRC()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RL()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RR()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SLA()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SRA()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SWAP()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SRL()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void BIT()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void RES()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };
	void SET()							{		throw std::runtime_error( "NOT IMPLEMENTED" ); };

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
