//
// Created by arthu on 18/09/2026.
//
#pragma once
#ifndef BUCKYBUCK_DEBUGDISPLAY_H
#define BUCKYBUCK_DEBUGDISPLAY_H
#include <memory>
#include "HexEditor_ImGUI.h"

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

	static std::unique_ptr<HexEditor_ImGUI>& GetHexEditor() { return m_oHexEditor; }

private:
	static DebugInfosDisplay*					m_pSingleton;

	GLFWwindow*									m_pWindow;
	static std::unique_ptr<HexEditor_ImGUI>		m_oHexEditor;
};


#endif //BUCKYBUCK_DEBUGDISPLAY_H
