//
// Created by arthu on 23/09/2026.
//

#include "ProcessorDebugDisplay.h"

#include <chrono>

#include "imgui.h"
#include "CPU.h"
#include "imgui_internal.h"
#include "IconsFontAwesome6.h"

#define REG_COLOR ImVec4( 0.337f, 0.612f, 0.839f, 1.00f )
#define DATA_COLOR ImVec4( 0.831f, 0.831f, 0.831f, 1.00f )
#define FLAG_COLOR ImVec4( 0.91f, 0.459f, 0.004f, 1.00f )
#define FLAG_ON_COLOR ImVec4( 0.094f, 0.808f, 0.09f, 1.00f )
#define U16_COLOR ImVec4( 1.00f, 0.902f, 0.051f, 1.00f )
#define FLOWCONTROL_CHILDSIZE 30.f

ProcessorDebugDisplay::ProcessorDebugDisplay() :
	m_pCPU( nullptr )
{
}

void ProcessorDebugDisplay::Update()
{
	if( m_pCPU == nullptr )
		m_pCPU = CPU::GetInstance();

	auto start = std::chrono::high_resolution_clock::now();

	static double iDurationMs;
	char titleBuffer[ 128 ];
	std::snprintf( titleBuffer,sizeof( titleBuffer ),"Processor (%.2f ms)###ProcDebugDisplayWindow",iDurationMs );

	if( ImGui::Begin( titleBuffer,nullptr ) )
	{
		_DrawFlowControl();

		m_oVisualParams.fScreenWidth = ImGui::GetWindowSize().x;
		m_oVisualParams.fDataSpace = ImGui::CalcTextSize( "FFF" ).x + 1.0f;

		ImVec2 pos = ImGui::GetCursorScreenPos();

		_DrawFlagsState( pos );
		_DrawRegistersState( pos );
	}
	ImGui::End();

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
}

void ProcessorDebugDisplay::_DrawFlagsState( ImVec2& pos )
{
	const char* sLabel = "Z  N  H  C";
	pos.x += ( m_oVisualParams.fScreenWidth - ImGui::CalcTextSize( sLabel ).x ) * 0.5f;
	_AddInfosText( "",sLabel,"",pos,0.0f,DATA_TEXT::TEXT_FLAG );

	_AddInfosText( m_pCPU->ZFlag,"","",pos,0.0f,DATA_TEXT::FLAG,NEW_LINE::COMPLETE_NEW );
	_AddInfosText( m_pCPU->NFlag,"","",pos,m_oVisualParams.fDataSpace,DATA_TEXT::FLAG );
	_AddInfosText( m_pCPU->HFlag,"","",pos,m_oVisualParams.fDataSpace * 2,DATA_TEXT::FLAG );
	_AddInfosText( m_pCPU->CFlag,"","",pos,m_oVisualParams.fDataSpace * 3,DATA_TEXT::FLAG,NEW_LINE::SAME_LINE,LINE::HORIZONTAL_LINE );
}

void ProcessorDebugDisplay::_DrawRegistersState( ImVec2& pos )
{
	pos.x = ImGui::GetCursorScreenPos().x + ( m_oVisualParams.fScreenWidth - ImGui::CalcTextSize( "PC    FFFFF" ).x ) * 0.5f;

	_AddInfosText( m_pCPU->GetPC(),"PC    ","$%04X",pos,0.0f,DATA_TEXT::U16,NEW_LINE::HALF_LINE,LINE::HORIZONTAL_LINE );
	_AddInfosText( m_pCPU->GetSP(),"SP    ","$%04X",pos,0.0f,DATA_TEXT::U16,NEW_LINE::HALF_LINE,LINE::HORIZONTAL_LINE );

	pos.x = ImGui::GetCursorScreenPos().x + ( m_oVisualParams.fScreenWidth * 0.5f );

	float fTextWidth = ImGui::CalcTextSize( "F       FFF" ).x * 0.5f;
	float fHalf = ( m_oVisualParams.fScreenWidth * 0.25f );
	float fLeftPart = -fHalf - fTextWidth;
	float fRightPart = fHalf - fTextWidth;

	_AddInfosText( m_pCPU->m_RegisterAF.hi,"A       ","$%02X",pos,fLeftPart,DATA_TEXT::REG,NEW_LINE::HALF_LINE,LINE::VERTICAL_LINE );
	_AddInfosText( m_pCPU->m_RegisterAF.lo,"F       ","$%02X",pos,fRightPart,DATA_TEXT::REG,NEW_LINE::SAME_LINE );
	_AddInfosText( m_pCPU->m_RegisterBC.hi,"B       ","$%02X",pos,fLeftPart,DATA_TEXT::REG,NEW_LINE::COMPLETE_NEW );
	_AddInfosText( m_pCPU->m_RegisterBC.lo,"C       ","$%02X",pos,fRightPart,DATA_TEXT::REG,NEW_LINE::SAME_LINE );
	_AddInfosText( m_pCPU->m_RegisterDE.hi,"D       ","$%02X",pos,fLeftPart,DATA_TEXT::REG,NEW_LINE::COMPLETE_NEW );
	_AddInfosText( m_pCPU->m_RegisterDE.lo,"E       ","$%02X",pos,fRightPart,DATA_TEXT::REG,NEW_LINE::SAME_LINE );
	_AddInfosText( m_pCPU->m_RegisterHL.hi,"H       ","$%02X",pos,fLeftPart,DATA_TEXT::REG,NEW_LINE::COMPLETE_NEW );
	_AddInfosText( m_pCPU->m_RegisterHL.lo,"L       ","$%02X",pos,fRightPart,DATA_TEXT::REG,NEW_LINE::SAME_LINE,LINE::HORIZONTAL_LINE );
}

void ProcessorDebugDisplay::_DrawFlowControl()
{
	ImGui::BeginChild( "##scrolling",ImVec2( 0.0f,FLOWCONTROL_CHILDSIZE ),false,ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav );

	ImGui::PushStyleVar( ImGuiStyleVar_FrameBorderSize,1.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding,3.0f );
	ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing,ImVec2( 8,4 ) );
	ImGui::PushStyleColor( ImGuiCol_Button,ImVec4( 0.08f,0.08f,0.08f,1.0f ) );

	const ImVec2 btn( 50,25 );
	const ImGuiStyle style = ImGui::GetCurrentContext()->Style;

	ImVec2& pos = ImGui::GetCurrentWindow()->DC.CursorPos;
	pos.x += ( ( m_oVisualParams.fScreenWidth - ( 50 + style.FramePadding.x * 2.0f ) * 4.0f ) * 0.5f );

	if( ImGui::Button( ICON_FA_PLAY,btn ) )				{	m_pCPU->Run(); }		ImGui::SameLine();
	if( ImGui::Button( ICON_FA_PAUSE,btn ) )			{	m_pCPU->Pause(); }		ImGui::SameLine();
	if( ImGui::Button( ICON_FA_FORWARD_STEP,btn ) || 
		ImGui::IsKeyPressed( ImGuiKey_F3 ) )			{	m_pCPU->SkipToNext(); } ImGui::SameLine();
	if( ImGui::Button( ICON_FA_ROTATE_LEFT,btn ) )		{	m_pCPU->Reset(); }

	ImGui::PopStyleVar( 3 );
	ImGui::PopStyleColor( 1 );

	ImGui::EndChild();
	ImGui::Separator();
}

ImVec4 ProcessorDebugDisplay::_GetColor( const DATA_TEXT oDataEnum )
{
	switch( oDataEnum )
	{
	case DATA_TEXT::TEXT:		return DATA_COLOR;	break;
	case DATA_TEXT::TEXT_FLAG:	return FLAG_COLOR;	break;
	case DATA_TEXT::FLAG:		return DATA_COLOR;	break;
	case DATA_TEXT::REG:		return REG_COLOR;	break;
	case DATA_TEXT::U16:		return U16_COLOR;	break;
	}
	return ImVec4();
}

template <typename T>
void ProcessorDebugDisplay::_AddInfosText( const T iVar,const char* sLabel,const char* sFormat,ImVec2& pos,float fXPos,const DATA_TEXT oDataEnum,NEW_LINE oNewLine /*= NEW_LINE( 0 )*/,LINE oAddLine /*= LINE::LINE*/ )
{
	ImDrawList* pDraw_list = ImGui::GetWindowDrawList();
	if( pDraw_list == nullptr )
		return;

	if( oNewLine != NEW_LINE::SAME_LINE )
		pos.y += oNewLine == NEW_LINE::COMPLETE_NEW ? m_oVisualParams.fNewLineHeight : m_oVisualParams.fNewLineHeight * 0.5f;

	switch( oDataEnum )
	{
	case DATA_TEXT::TEXT:
	case DATA_TEXT::TEXT_FLAG:
		pDraw_list->AddText( ImVec2( pos.x + fXPos,pos.y ),ImGui::GetColorU32( _GetColor( oDataEnum ) ),sLabel );
		break;
	case DATA_TEXT::FLAG:
		pDraw_list->AddText( ImVec2( pos.x + fXPos,pos.y ),iVar ? ImGui::GetColorU32( FLAG_ON_COLOR ) : ImGui::GetColorU32( _GetColor( oDataEnum ) ),iVar ? "1" : "0" );
		break;
	case DATA_TEXT::U16:
	case DATA_TEXT::REG:
	{
		char aBuffer[ 12 ] = "";
		ImFormatString( aBuffer,sizeof( aBuffer ),sFormat,iVar );
		pDraw_list->AddText( ImVec2( pos.x + fXPos,pos.y ),ImGui::GetColorU32( _GetColor( oDataEnum ) ),sLabel );
		pDraw_list->AddText( ImVec2( pos.x + ImGui::CalcTextSize( sLabel ).x + fXPos,pos.y ),ImGui::GetColorU32( DATA_COLOR ),aBuffer );
	}
	break;
	}

	if( oAddLine == LINE::HORIZONTAL_LINE )
	{
		float fPosX = ImGui::GetCursorScreenPos().x;
		pos.y += m_oVisualParams.fNewLineHeight;
		pDraw_list->AddLine( ImVec2( fPosX,pos.y ),ImVec2( fPosX + 9999,pos.y ),ImGui::GetColorU32( ImGuiCol_Border ) );
	}
	else if( oAddLine == LINE::VERTICAL_LINE )
	{
		float fPosX = ImGui::GetCursorScreenPos().x + ( m_oVisualParams.fScreenWidth * 0.5f );
		float fPosY = pos.y - ( oNewLine == NEW_LINE::COMPLETE_NEW ? m_oVisualParams.fNewLineHeight : m_oVisualParams.fNewLineHeight * 0.5f );
		pDraw_list->AddLine( ImVec2( fPosX,fPosY ),ImVec2( fPosX,fPosY + 9999 ),ImGui::GetColorU32( ImGuiCol_Border ) );
	}
}
