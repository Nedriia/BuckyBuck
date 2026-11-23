//
// Created by arthu on 05/11/2026.
//

#pragma once

#ifndef BUCKYBUCK_VRAMVIEWER_H
#define BUCKYBUCK_VRAMVIEWER_H

#include <cstdint>
#include <glad/glad.h>
#include "Shader.h"
#include <memory>
#include <array>

enum Tab
{
	BGMAP,
	TILES,
	OAM,
	PALETTE,
	COUNT
};

class IEVRAMViewerOption;
class VRAMViewer
{
public:
	VRAMViewer();
	~VRAMViewer();

	void Update();
	unsigned int GetFBOTexture() const { return m_iFBOTexture; }
	void UseShader() { m_sShaderProgram.Use(); }

private:
	void								_InitFBOTexture();
	void								_InitFramebuffer();
	void								_InitRenderer();
	void								_DestroyRenderer();

	Shader 								m_sShaderProgram;

	unsigned int						m_iFBOTexture;
	unsigned int 						m_iVAO;
	unsigned int 						m_iVBO;
	unsigned int 						m_iEBO;
	unsigned int						m_iFBO;

	std::array<std::unique_ptr<IEVRAMViewerOption>,Tab::COUNT>	m_aTabOption;
};

class IEVRAMViewerOption
{
	public :
		IEVRAMViewerOption();
		virtual		~IEVRAMViewerOption();
		virtual		void InitTexture() = 0;
		virtual		void UpdateTexture( VRAMViewer& oVRAMViewer ) = 0;
		virtual		void RenderTexture( VRAMViewer& oVRAMViewer ) = 0;
		virtual		void DetermineTileSelection() = 0;

	protected:
		bool								m_bDrawGrid;
		static uint16_t						m_iAdressSelected;
		static int							m_iHoveredLine;
		static int							m_iHoveredCol;

		unsigned int 						m_iTexture;
};

class BGMapTab : public IEVRAMViewerOption
{
	public:
		void		InitTexture() override {};
		void		UpdateTexture( VRAMViewer& oVRAMViewer )override {};
		void		RenderTexture( VRAMViewer& oVRAMViewer )override {};
		void		DetermineTileSelection() override {};

	private:
		uint8_t m_iBGMapDataPixels[ 256 ][ 256 ];
};

class TilesTab : public IEVRAMViewerOption
{
	public:
		TilesTab();
		void		InitTexture() override;
		void		UpdateTexture( VRAMViewer& oVRAMViewer ) override;
		void		RenderTexture( VRAMViewer& oVRAMViewer ) override;
		void		DetermineTileSelection() override;
	private:
		void		_GetVRAMData();
		uint8_t		m_iTilesDataPixels[ 192 ][ 128 ];
};

class OAMTab : public IEVRAMViewerOption
{
	public:
		void		InitTexture() override {};
		void		UpdateTexture( VRAMViewer& oVRAMViewer )override {};
		void		RenderTexture( VRAMViewer& oVRAMViewer )override {};
		void		DetermineTileSelection() override {};
};

class PaletteTab : public IEVRAMViewerOption
{
	public:
		void		InitTexture() override {};
		void		UpdateTexture( VRAMViewer& oVRAMViewer )override {};
		void		RenderTexture( VRAMViewer& oVRAMViewer )override {};
		void		DetermineTileSelection() override {};
};


#endif //BUCKYBUCK_VRAMVIEWER_H
