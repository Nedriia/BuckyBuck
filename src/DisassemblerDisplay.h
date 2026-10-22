//
// Created by arthu on 23/09/2026.
//

#pragma once

#ifndef BUCKYBUCK_DISASSEMBLERDISPLAY_H
#define BUCKYBUCK_DISASSEMBLERDISPLAY_H
#include <cstdint>

struct ImDrawList;
class DisassemblerDisplay
{
public:
	DisassemblerDisplay();
	~DisassemblerDisplay(){};

	void Update();

protected :

	void				_SelectLine( const int iStart, const int iEnd );
	void				_DrawOptions();
	void				_DrawSelectedLine( ImDrawList* draw_list, const float fWindowPosX, const float fWindowPosY, const uint16_t iStart, const uint16_t iSize );
	static const char*	_EndOfNthBlock( const char* text, int nb_blocs );

	int					m_iSelectedLine;
	bool				m_bFollowPc;
	uint16_t			m_iLastPc;
};


#endif //BUCKYBUCK_DISASSEMBLERDISPLAY_H
