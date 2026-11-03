//
// Created by arthu on 03/11/2026.
//

#include "Timer.h"
#include "CPU.h"

#define DIV  0xFF04
#define TIMA 0xFF05
#define TMA  0xFF06
#define TAC  0xFF07

Timer* Timer::m_pSingleton = nullptr;

uint16_t m_iInternalCounter = 0;

Timer::Timer()	:
	m_iTIMA( 0 ), //TODO : Reset
	m_iTimerCounter( 0 )
{
}

Timer::~Timer()
{
	m_pSingleton = nullptr;
}

void Timer::UpdateTimers( const int iCycles )
{
	_UpdateDividerRegister( iCycles ); //4MHZ / 16384Hz = 256 //TODO : Double speed

	if( ( _GetTimerControl() & 0x4 ) == 0 ) return;

	m_iTIMA -= iCycles;
	while( m_iTIMA <= 0 )
	{
		m_iTIMA += _GetTimerSpeed();

		uint8_t iValue = _GetTimerCounter();
		if( iValue == 0xFF )
		{
			CPU::GetInstance()->WriteMemory( TIMA,_GetTimerModulo() );
			CPU::GetInstance()->RequestInterrupt( 2 );
		}
		else
		{
			CPU::GetInstance()->GetRefAtMemory( TIMA )++;
		}
	}
}

void Timer::_UpdateDividerRegister(const int iCycle )
{
	m_iInternalCounter += iCycle;
	CPU::GetInstance()->GetRefAtMemory( DIV ) = m_iInternalCounter >> 8;
}

uint8_t Timer::_GetClockFrequency()
{
	uint8_t iTimerControl = _GetTimerControl();
	return ( iTimerControl & 0x3 );
}

int Timer::_GetTimerSpeed()
{
	static const int periods[ 4 ] = { 1024, 16, 64, 256 };
	return periods[ _GetTimerControl() & 0x3 ];
}

void Timer::SetClockFrequency()
{
	uint8_t iTimerControl = _GetClockFrequency();
	switch( iTimerControl )
	{
		case 0: m_iTIMA = 1024; break;
		case 1: m_iTIMA = 16; break;
		case 2: m_iTIMA = 64; break;
		case 3: m_iTIMA = 256; break;
	}
}

uint8_t Timer::_GetDividerRegister()
{
	return CPU::GetInstance()->ReadMemory( DIV );
}

uint8_t Timer::_GetTimerCounter()
{
	return CPU::GetInstance()->ReadMemory( TIMA );
}

uint8_t Timer::_GetTimerModulo()
{
	return CPU::GetInstance()->ReadMemory( TMA );
}

uint8_t Timer::_GetTimerControl()
{
	return CPU::GetInstance()->ReadMemory( TAC );
}

void Timer::SetResetDivider()
{
	m_iInternalCounter = 0;
}

void Timer::SetTimerCounter( const uint8_t iValue )
{
	CPU::GetInstance()->WriteMemory( TIMA, iValue );
	m_iTIMA = iValue;
}