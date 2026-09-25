//
// Created by arthu on 23/09/2026.
//

#include "DisassemblerDisplay.h"

#include <chrono>
#include <GLFW/glfw3.h>

#include "CPU.h"
#include "Disassembler.h"
#include "imgui.h"

void DisassemblerDisplay::Update()
{
	auto start = std::chrono::high_resolution_clock::now();

	glfwPollEvents();
	static double iDurationMs;
	char titleBuffer[ 128 ];
	std::snprintf( titleBuffer,sizeof( titleBuffer ),"Dissambler (%.2f ms)###DisassemblerDisplayWindow",iDurationMs );
	if( ImGui::Begin( titleBuffer,nullptr ) )
	{
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetWindowPos();
		ImGuiStyle& style = ImGui::GetStyle();
		pos.x += ImGui::CalcTextSize( "F" ).x + 1.0f;
		pos.y += ImGui::GetTextLineHeight() * 2.0f;

		CPU* m_pCpuInstance = CPU::GetInstance();
		const auto& aDisassemblyInstructions = Disassembler::GetDisassemblyInstructions();
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

		int num_items = static_cast< int >( aDisassemblyInstructions.size() );
		ImGuiListClipper clipper;
		clipper.Begin( num_items,15.0f * style.FontScaleDpi );

		while( clipper.Step() )
		{
			for( int n = clipper.DisplayStart; n < clipper.DisplayEnd; ++n )
			{
				auto oInstruct = aDisassemblyInstructions.at( n );
				// bool isCurrent = m_iAddress == m_pCPU->GetPC();
				//
				// if( isCurrent )
				// 	ImGui::PushStyleColor( ImGuiCol_Text,ImVec4( 1,1,0,1 ) );

				draw_list->AddText( pos,ImGui::GetColorU32( ImGuiCol_Text ), oInstruct.m_sText.c_str() );
				pos.y += 15.0f * style.FontScaleDpi;
				//
				// if( isCurrent )
				// 	ImGui::PopStyleColor();
			}
		}
	}
	ImGui::End();

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
}
