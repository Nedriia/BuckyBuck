//
// Created by arthu on 18/09/2026.
//
#pragma once
#ifndef BUCKYBUCK_DEBUGDISPLAY_H
#define BUCKYBUCK_DEBUGDISPLAY_H
#include <memory>
#include "HexEditor_ImGUI.h"
#include "ProcessorDebugDisplay.h"
#include "DisassemblerDisplay.h"

class DebugInfosDisplay
{
public:
	DebugInfosDisplay();
	~DebugInfosDisplay();

	void Init( GLFWwindow* mainWindow );
	void StartFrame();
	void Render();
	void Update();
	void Destroy();

	static DebugInfosDisplay* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new DebugInfosDisplay;
		return m_pSingleton;
	}

	static std::unique_ptr<HexEditor_ImGUI>&		GetHexEditor() { return m_oHexEditor; }
	static std::unique_ptr<ProcessorDebugDisplay>&	GetProcDebugDisplay() { return m_oProcDebugDisplay; }
	static std::unique_ptr<DisassemblerDisplay>&	GetDisassemblerDisplay() { return m_oDisassemblerDisplay; }

private:
	static DebugInfosDisplay*						m_pSingleton;

	GLFWwindow*										m_pWindow;
	static std::unique_ptr<HexEditor_ImGUI>			m_oHexEditor;
	static std::unique_ptr<ProcessorDebugDisplay>	m_oProcDebugDisplay;
	static std::unique_ptr<DisassemblerDisplay>		m_oDisassemblerDisplay;
};


#endif //BUCKYBUCK_DEBUGDISPLAY_H
