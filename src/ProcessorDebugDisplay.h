//
// Created by arthu on 23/09/2026.
//
#pragma once

#ifndef BUCKYBUCK_PROCESSORDEBUGDISPLAY_H
#define BUCKYBUCK_PROCESSORDEBUGDISPLAY_H
#include <cstdint>
#include <imgui.h>


enum DATA_TEXT
{
	TEXT,
	TEXT_FLAG,
	U16,
	FLAG,
	REG
};

enum LINE
{
	NONE,
	HORIZONTAL_LINE,
	VERTICAL_LINE,
};

enum NEW_LINE
{
	SAME_LINE,
	HALF_LINE,
	COMPLETE_NEW,
};

class CPU;
class ProcessorDebugDisplay
{

public:

	ProcessorDebugDisplay();
	~ProcessorDebugDisplay(){};

	void Update();

private:

	void _DrawFlagsState( ImVec2& pos );
	void _DrawRegistersState( ImVec2& pos );
	void _DrawFlowControl();

	template <typename T>
	void _AddInfosText( const T iVar,const char* sLabel,const char* sFormat,ImVec2& pos, float fXPos, const DATA_TEXT oDataEnum,NEW_LINE oNewLine = NEW_LINE( 0 ),LINE oAddLine = LINE( 0 ) );

	ImVec4 _GetColor( const DATA_TEXT oDataEnum );

	struct VisualParam
	{
		ImGuiStyle& style = ImGui::GetStyle();

		float fNewLineHeight	= 25.0f * style.FontScaleDpi;
		float fCharSpace		= 15.0f * style.FontScaleDpi;
		float fScreenWidth		= 0.0f;
		float fDataSpace		= 0.0f;
	};

	VisualParam m_oVisualParams;

	CPU*		m_pCPU;
};


#endif //BUCKYBUCK_PROCESSORDEBUGDISPLAY_H
