//
// Created by arthu on 23/09/2026.
//

#include "DisassemblerDisplay.h"

#include <chrono>
#include <GLFW/glfw3.h>

#include "CPU.h"
#include "Disassembler.h"
#include "imgui.h"
#include "imgui_internal.h"

#define ADDR_COLOR ImVec4( 0.082f, 0.573f, 0.573f, 1.00f )
#define DATA_COLOR ImVec4( 0.953f, 0.478f, 0.918f, 1.00f )
#define DATA_BIS_COLOR ImVec4( 0.953f, 0.478f, 0.918f, 1.00f )
#define LENGTH_COLOR ImVec4( 0.953f, 0.478f, 0.918f, 1.00f )
#define MNEMONIC_COLOR ImVec4( 0.88f, 0.796f, 0.051f, 1.00f )
#define ADITIONNALINFO_COLOR ImVec4( 0.867f, 0.439f, 0.008f, 1.00f )
#define COMMENT_COLOR ImVec4( 0.867f, 0.439f, 0.008f, 1.00f )

#define START_ADDR_POS		float( 10.0f )
#define START_DATA_POS		float( START_ADDR_POS + 100.0f )
#define START_MNEMONIC_POS	float( START_DATA_POS + 125.0f )
#define START_DATA_ADD_POS	float( START_MNEMONIC_POS + 150.0f )
#define START_DURATION_POS	float( START_DATA_ADD_POS + 150.0f )
#define START_ADDINFO_POS	float( START_DURATION_POS + 400.0f )
#define START_COMMENT_POS	float( START_ADDINFO_POS + 275.0f )

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

		//CPU* m_pCpuInstance = CPU::GetInstance();
		// if( m_bFollowPc )
		// {
		// 	std::vector<uint16_t>::iterator it = std::lower_bound( m_aAdress.begin(),m_aAdress.end(),m_pCPU->GetPC() );
		// 	if( it != m_aAdress.end() )
		// 	{
		// 		int iIndex = std::distance( m_aAdress.begin(),it );
		// 		if( iIndex >= 0 )
		// 		{
		// 			float scroll_target = iIndex * ImGui::GetTextLineHeightWithSpacing() - ImGui::GetWindowHeight() * 0.5f;
		// 			ImGui::SetScrollY( scroll_target );
		// 		}
		// 	}
		// }

		const auto& aDisassemblyInstructions = Disassembler::GetDisassemblyInstructions();
		int num_items = static_cast< int >( aDisassemblyInstructions.size() );
		ImGuiListClipper clipper;
		clipper.Begin( num_items,15.0f * style.FontScaleDpi );

		ImVec2 pos;
		pos.y = window_pos.y + ImGui::GetTextLineHeight() * 2.0f;

		while( clipper.Step() )
		{
			for( int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n )
			{
				auto oInstruct = aDisassemblyInstructions.at( n );
				//missing registers / flags / instructions count
				char aBuffer[128] = "";

				pos.x = window_pos.x + START_ADDR_POS * style.FontScaleDpi;
				ImFormatString( aBuffer, sizeof(aBuffer), "%s", oInstruct.m_iAdress.c_str() );
				draw_list->AddText( pos,ImGui::GetColorU32( ADDR_COLOR ), aBuffer );

				pos.x = window_pos.x + START_DATA_POS * style.FontScaleDpi;
				ImFormatString( aBuffer, sizeof( aBuffer ), "%s",  oInstruct.m_oData.c_str() );
				const char* sEnd = DisassemblerDisplay::EndOfNthBlock( aBuffer, 4 );
				draw_list->AddText( pos,ImGui::GetColorU32( DATA_COLOR ), aBuffer, sEnd );
				if ( ( *sEnd ) != '\0' )
				{
					ImVec2 sz = ImGui::CalcTextSize( aBuffer, sEnd );
					draw_list->AddText( ImVec2( pos.x + sz.x, pos.y ),
										ImGui::GetColorU32( DATA_COLOR ), "+" );
				}

				pos.x = window_pos.x + START_DATA_ADD_POS * style.FontScaleDpi;//Need to adapt
				ImFormatString( aBuffer, sizeof( aBuffer ), "%s",  oInstruct.m_oData.c_str() );
				draw_list->AddText( pos,ImGui::GetColorU32( DATA_BIS_COLOR ), aBuffer );

				if ( oInstruct.m_iDuration != 0xFF )
				{
					pos.x = window_pos.x + START_DURATION_POS * style.FontScaleDpi;
					ImFormatString( aBuffer, sizeof( aBuffer ), "%i",  oInstruct.m_iDuration );
					draw_list->AddText( pos,ImGui::GetColorU32( LENGTH_COLOR ), aBuffer );
				}

				pos.x = window_pos.x + START_MNEMONIC_POS * style.FontScaleDpi;
				ImFormatString( aBuffer, sizeof(aBuffer), "%s", oInstruct.m_sMnemonic.c_str() );
				draw_list->AddText( pos,ImGui::GetColorU32( MNEMONIC_COLOR ), aBuffer );

				pos.x = window_pos.x + START_ADDINFO_POS * style.FontScaleDpi;
				ImFormatString( aBuffer, sizeof(aBuffer), "%s", oInstruct.m_sAditionalInfo.c_str() );
				draw_list->AddText( pos,ImGui::GetColorU32( ADITIONNALINFO_COLOR ), aBuffer );

				pos.x = window_pos.x + START_COMMENT_POS * style.FontScaleDpi;
				ImFormatString( aBuffer, sizeof(aBuffer), "%s", oInstruct.m_sComment.c_str() );
				draw_list->AddText( pos,ImGui::GetColorU32( COMMENT_COLOR ), aBuffer );

				pos.y += 15.0f * style.FontScaleDpi;
			}
		}
	}
	ImGui::End();

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
}

const char* DisassemblerDisplay::EndOfNthBlock( const char* text, int nb_blocs )
{
	const char* p = text;
	int blocs = 0;
	while ( *p )
	{
		if ( *p == ' ' )
		{
			if ( ++blocs == nb_blocs )
				return p;
		}
		++p;
	}
	return p;
}
