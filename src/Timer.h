//
// Created by arthu on 03/11/2026.
//

#ifndef BUCKYBUCK_TIMER_H
#define BUCKYBUCK_TIMER_H

#include <cstdint>

class Timer
{
public:
	Timer();
	~Timer();
	void UpdateTimers( const int iCycle );
	void SetClockFrequency();

	void SetResetDivider();
	void SetTimerCounter( const uint8_t iValue );

	static Timer* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new Timer;
		return m_pSingleton;
	}
	void	DestroyInstance() { delete m_pSingleton; }

private:
	void _UpdateDividerRegister( const int iCycle );
	
	uint8_t _GetClockFrequency();
	int _GetTimerSpeed();

	uint8_t _GetDividerRegister();
	uint8_t _GetTimerCounter();
	uint8_t _GetTimerModulo();
	uint8_t _GetTimerControl();

	static Timer*	m_pSingleton;
	int				m_iTIMA;
	int				m_iTimerCounter;
};


#endif //BUCKYBUCK_TIMER_H
