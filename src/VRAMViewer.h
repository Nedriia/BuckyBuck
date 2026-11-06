//
// Created by arthu on 05/11/2026.
//

#pragma once

#ifndef BUCKYBUCK_VRAMVIEWER_H
#define BUCKYBUCK_VRAMVIEWER_H

#include <cstdint>
#include <glad/glad.h>
#include "Shader.h"

class VRAMViewer
{
public:
	VRAMViewer();
	~VRAMViewer();

	void Update();

private:
	void _InitTexture();
	void _InitFramebuffer();
	void _InitRenderer();
	void _DestroyRenderer();

	void _GetVRAMData();
	void _UpdateTexture();
	void _DisplayTexture();
	uint8_t m_iVRAMPixels[136][128];

	static unsigned int 				m_iVRAMTexture;
	Shader 								m_sShaderProgram;

	static unsigned int					m_iFBOTexture;
	unsigned int 						m_iVAO;
	unsigned int 						m_iVBO;
	unsigned int 						m_iEBO;
	unsigned int						m_iFBO;
};


#endif //BUCKYBUCK_VRAMVIEWER_H
