//
// Created by arthu on 23/09/2026.
//

#include "DisassemblerDisplay.h"

#include <chrono>
#include <GLFW/glfw3.h>

#include "Disassembler.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "CPU.h"
#include "IconsFontAwesome6.h"

#define ADDR_COLOR ImVec4( 0.337f, 0.612f, 0.839f, 1.00f )
#define DATA_COLOR ImVec4( 0.831f, 0.831f, 0.831f, 1.00f )
#define DATA_BIS_COLOR ImVec4( 0.678f, 0.502f, 0.361f, 1.00f )
#define LENGTH_COLOR ImVec4( 0.93f, 0.537f, 0.208f, 1.00f )
#define MNEMONIC_COLOR ImVec4( 0.306f, 0.788f, 0.69f, 1.00f )
#define ADITIONNALINFO_COLOR ImVec4( 0.357f, 0.357f, 0.357f, 1.00f )
#define BREAKPOINT_COLOR ImVec4( 1.0f, 0.0f, 0.0f, 1.00f )

#define START_BREAKPOINT_POS	float( 10.0f )
#define START_ADDR_POS			float( START_BREAKPOINT_POS + 20.0f )
#define START_DATA_POS			float( START_ADDR_POS + 105.0f )
#define START_MNEMONIC_POS		float( START_DATA_POS + 125.0f )
#define START_DATA_ADD_POS		float( START_MNEMONIC_POS + 175.0f )
#define START_DURATION_POS		float( START_DATA_ADD_POS + 115.0f )
#define START_ADDINFO_POS		float( START_DURATION_POS + 50.0f )

DisassemblerDisplay::DisassemblerDisplay() :
	m_iSelectedLine( -1 ),
	m_bFollowPc( true ),
	m_iLastPc( 0 )
{

}

void DisassemblerDisplay::Update()
{
	auto start = std::chrono::high_resolution_clock::now();

	glfwPollEvents();
	static double iDurationMs;
	char titleBuffer[ 128 ];
	std::snprintf( titleBuffer,sizeof( titleBuffer ),"Disassembler (%.2f ms)###DisassemblerDisplayWindow",iDurationMs );
	if( ImGui::Begin( titleBuffer,nullptr ) )
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 window_pos = ImGui::GetWindowPos();

		ImGuiStyle& style = ImGui::GetStyle();

		const auto& aDisassemblyInstructions = Disassembler::GetDisassemblyInstructions();

		CPU* pCpu = CPU::GetInstance();
		if( pCpu )
		{
			if( m_iLastPc != pCpu->GetPC() )
			{
				const uint16_t iAdress = pCpu->GetPC();
				for( auto it = aDisassemblyInstructions.begin(); it != aDisassemblyInstructions.end(); ++it )
				{
					if( ( *it ).m_iAdress == iAdress )
					{
						int iIndex = std::distance( aDisassemblyInstructions.begin(),it );
						if( iIndex >= 0 )
						{
							m_iSelectedLine = iIndex;
							if( m_bFollowPc )
							{
								float scroll_target = ( iIndex + 1 ) * ( 15.0f * style.FontScaleDpi );
								ImGui::SetScrollY( scroll_target );
							}
						}
						m_iLastPc = pCpu->GetPC();
						break;
					}
				}
			}
		}

		int num_items = static_cast< int >( aDisassemblyInstructions.size() );
		ImGuiListClipper clipper;
		clipper.Begin( num_items,15.0f * style.FontScaleDpi );

		ImVec2 pos;
		pos.y = window_pos.y + ImGui::GetTextLineHeight() * 2.0f;

		while( clipper.Step() )
		{
			_SelectLine( clipper.DisplayStart, clipper.DisplayEnd );
			_DrawSelectedLine( draw_list,window_pos.x,window_pos.y,clipper.DisplayStart,clipper.DisplayEnd );

			for( int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n )
			{
				auto& oInstruct = aDisassemblyInstructions[ n ];
				//missing registers / flags / instructions count
				char aBuffer[ 128 ] = "";

				pos.x = window_pos.x + START_BREAKPOINT_POS * style.FontScaleDpi;
				if( pCpu && pCpu->IsBreakpoint( oInstruct.m_iAdress ) )
					draw_list->AddText( pos,ImGui::GetColorU32( BREAKPOINT_COLOR ),ICON_FA_CIRCLE );

				pos.x = window_pos.x + START_ADDR_POS * style.FontScaleDpi;
				ImFormatString( aBuffer,sizeof( aBuffer ),"%s::%#06X",oInstruct.m_aLabel,oInstruct.m_iAdress );
				draw_list->AddText( pos,ImGui::GetColorU32( ADDR_COLOR ),aBuffer );

				pos.x = window_pos.x + START_DATA_POS * style.FontScaleDpi;
				ImFormatString( aBuffer,sizeof( aBuffer ),"%s",oInstruct.m_aData );
				const char* sEnd = DisassemblerDisplay::_EndOfNthBlock( aBuffer,4 );
				draw_list->AddText( pos,ImGui::GetColorU32( DATA_COLOR ),aBuffer,sEnd );
				if( ( *sEnd ) != '\0' )
				{
					ImVec2 sz = ImGui::CalcTextSize( aBuffer,sEnd );
					draw_list->AddText( ImVec2( pos.x + sz.x,pos.y ),
						ImGui::GetColorU32( DATA_COLOR ),"+" );
				}

				pos.x = window_pos.x + START_DATA_ADD_POS * style.FontScaleDpi;
				if( oInstruct.m_iAdress != 0x134 )
				{
					ImFormatString( aBuffer,sizeof( aBuffer ),"%s",oInstruct.m_aDataBis );
					draw_list->AddText( pos,ImGui::GetColorU32( DATA_BIS_COLOR ),aBuffer );
					pos.x += ImGui::CalcTextSize( "FF" ).x + 1.0f;

					pos.x = window_pos.x + START_MNEMONIC_POS * style.FontScaleDpi;
					ImFormatString( aBuffer,sizeof( aBuffer ),"%s",oInstruct.m_sMnemonic );
					draw_list->AddText( pos,ImGui::GetColorU32( MNEMONIC_COLOR ),aBuffer );
				}
				else
				{
					ImFormatString( aBuffer, sizeof( aBuffer ), "%s", oInstruct.m_sMnemonic );
					draw_list->AddText( pos,ImGui::GetColorU32( DATA_BIS_COLOR ), aBuffer );
				}

				if ( oInstruct.m_iAdress != 0xFF && oInstruct.m_iDuration )
				{
					pos.x = window_pos.x + START_DURATION_POS * style.FontScaleDpi;
					ImFormatString( aBuffer, sizeof( aBuffer ), "%i", oInstruct.m_iDuration );
					draw_list->AddText( pos,ImGui::GetColorU32( LENGTH_COLOR ), aBuffer );
				}

				pos.x = window_pos.x + START_ADDINFO_POS * style.FontScaleDpi;
				ImFormatString( aBuffer, sizeof( aBuffer ), "%s", oInstruct.m_sAditionalInfo );
				draw_list->AddText( pos,ImGui::GetColorU32( ADITIONNALINFO_COLOR ), aBuffer );

				pos.y += 15.0f * style.FontScaleDpi;
			}
		}
	}
	_DrawOptions();

	ImGui::End();

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
}

void DisassemblerDisplay::_SelectLine( const int iStart,const int iEnd )
{
	if( ImGui::IsMouseClicked( 0 ) && ImGui::IsPopupOpen( "context" ) == false )
	{
		ImVec2 mouse_pos = ImGui::GetMousePos();
		ImVec2 window_pos = ImGui::GetWindowPos();
		ImVec2 window_size = ImGui::GetWindowSize();

		ImVec2 zone_min(
			window_pos.x,
			window_pos.y + ( ImGui::GetTextLineHeight() * 2.0f )
		);

		ImVec2 zone_max(
			window_pos.x + window_size.x,
			window_pos.y + window_size.y
		);

		if( ImGui::IsMouseHoveringRect( zone_min,zone_max ) )
		{
			ImGuiStyle& style = ImGui::GetStyle();

			//Now check if the mouse is on data
			float gridStartX = window_pos.x;
			float gridStartY = window_pos.y + ImGui::GetTextLineHeightWithSpacing();

			float relativeX = mouse_pos.x - gridStartX;
			float relativeY = mouse_pos.y - gridStartY;

			if( relativeX >= 0 && relativeY >= 0 )
			{
				int hoveredLine = static_cast< int >( relativeY / ( 15.0f * style.FontScaleDpi ) );
				m_iSelectedLine = iStart + hoveredLine;
			}
		}
	}

	if( ImGui::IsWindowFocused( ImGuiFocusedFlags_ChildWindows ) )
	{
		if( m_iSelectedLine != -1 )
		{
			int iLine = m_iSelectedLine;
			if( ImGui::IsKeyPressed( ImGuiKey_UpArrow ) ) { iLine--; }
			else if( ImGui::IsKeyPressed( ImGuiKey_DownArrow ) ) { iLine++; }

			iLine = std::max( 0,iLine );
			iLine = std::min( iLine,static_cast< int >( Disassembler::GetDisassemblyInstructions().size() ) );

			if( iLine != m_iSelectedLine )
			{
				//Scroll to focus
				ImGuiStyle& style = ImGui::GetStyle();
				float selectedY = iLine * ( 15.0f * style.FontScaleDpi );
				if( iLine < iStart )
					ImGui::SetScrollFromPosY( ImGui::GetCursorStartPos().y + selectedY,0.0f );
				else if( iLine > iEnd )
					ImGui::SetScrollFromPosY( ImGui::GetCursorStartPos().y + selectedY,1.0f );

				m_iSelectedLine = iLine;
			}
		}
	}

	if( ImGui::IsMouseClicked( 1 ) )
	{
		//Check if mouse is on current window and focus on it
		ImVec2 mouse_pos = ImGui::GetMousePos();
		ImVec2 window_pos = ImGui::GetWindowPos();
		ImVec2 window_size = ImGui::GetWindowSize();

		ImVec2 zone_min(
			window_pos.x,
			window_pos.y
		);

		ImVec2 zone_max(
			window_pos.x + window_size.x,
			window_pos.y + window_size.y
		);

		if( ImGui::IsMouseHoveringRect( zone_min,zone_max ) )
			ImGui::OpenPopup( "context" );
	}

	if( ImGui::IsKeyPressed( ImGuiKey_F9 ) ) 
	{
		const auto& aDisassemblyInstructions = Disassembler::GetDisassemblyInstructions();
		CPU::GetInstance()->MarkBreakpoint( aDisassemblyInstructions[ m_iSelectedLine ].m_iAdress );
	}
}

void DisassemblerDisplay::_DrawOptions()
{
	if( ImGui::BeginPopup( "context" ) )
	{
		ImGui::Checkbox( "Follow PC",&m_bFollowPc );
		ImGui::EndPopup();
	}
}

void DisassemblerDisplay::_DrawSelectedLine( ImDrawList* draw_list,const float fWindowPosX,const float fWindowPosY,const uint16_t iStart,const uint16_t iSize )
{
	if( m_iSelectedLine != -1 )
	{
		ImGuiStyle& style = ImGui::GetStyle();

		//Check if current line selection is in current viewport
		if( m_iSelectedLine < iStart || m_iSelectedLine >( iStart + iSize ) )
			return;

		ImVec2 vStartPos = { fWindowPosX, fWindowPosY + ImGui::GetTextLineHeight() * 2.0f };
		vStartPos.y += ( m_iSelectedLine - iStart ) * ( 15.0f * style.FontScaleDpi );

		draw_list->AddRectFilled( vStartPos,ImVec2( vStartPos.x + 99999,vStartPos.y + ImGui::GetTextLineHeight() ),ImGui::GetColorU32( ImGuiCol_DockingPreview ) );
	}
}

const char* DisassemblerDisplay::_EndOfNthBlock( const char* text,int nb_blocs )
{
	const char* p = text;
	int blocs = 0;
	while( *p )
	{
		if( *p == ' ' )
		{
			if( ++blocs == nb_blocs )
				return p;
		}
		++p;
	}
	return p;
}
