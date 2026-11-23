//
// Created by arthu on 05/11/2026.
//

#ifndef BUCKYBUCK_PPU_H
#define BUCKYBUCK_PPU_H
#include <cstdint>

class PPU
{
public:

	void Update( const int iCycles );
	void Destroy(){ delete m_pSingleton ;}
	static PPU* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new PPU;
		return m_pSingleton;
	}

private:
	PPU();
	~PPU();

	static PPU* m_pSingleton;
	uint8_t m_pPixels[160][144];
};


#endif //BUCKYBUCK_PPU_H
