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
		IEVRAMViewerOption( uint16_t XSize,uint16_t YSize, int TilesLine, int TilesCol, int SizeTile );
		virtual		void InitTexture();
		virtual		void RenderTexture( VRAMViewer& oVRAMViewer );

		virtual		~IEVRAMViewerOption();
		virtual		void UpdateTexture( VRAMViewer& oVRAMViewer ) = 0;
		virtual		void DrawOptions( VRAMViewer& oVRAMViewer ) = 0;
		virtual		void DetermineTileSelection() = 0;

	protected:
		static int							m_iHoveredLine;
		static int							m_iHoveredCol;

		unsigned int 						m_iTexture;

		int									m_iXSizeTexture;
		int									m_iYSizeTexture;

		int									m_iTilesPerLine;
		int									m_iTilesPerCol;
		int									m_iSizeTile;

		float								m_fxOffsetGrid;
		float								m_fyOffsetGrid;

		bool								m_bDrawGrid;
		uint16_t							m_iAdressSelected;
};

class BGMapTab : public IEVRAMViewerOption
{
	public:
		BGMapTab();
		void		UpdateTexture( VRAMViewer& oVRAMViewer )override;
		void		DetermineTileSelection() override;
		void		DrawOptions( VRAMViewer& oVRAMViewer ) override;

	private:
		void		_SetTilesBGData();
		uint8_t		m_iBGMapDataPixels[ 256 ][ 256 ];

		bool		m_bScyx;
		bool		m_bPal;

		enum MapDetail
		{
			AutoM,
			$9800,
			$9C00
		};

		enum TilesDetail
		{
			AutoT,
			$8800,
			$8000
		};

		uint8_t		m_iMapDetail;
		uint8_t		m_iTilesDetail;
};

class TilesTab : public IEVRAMViewerOption
{
	public:
		TilesTab();
		void		UpdateTexture( VRAMViewer& oVRAMViewer ) override;
		void		DetermineTileSelection() override;
		void		DrawOptions( VRAMViewer& oVRAMViewer ) override;
	private:
		void		_SetTilesData();
		uint8_t		m_iTilesDataPixels[ 192 ][ 128 ];
};

class OAMTab : public IEVRAMViewerOption
{
	public:
		void		UpdateTexture( VRAMViewer& oVRAMViewer )override {};
		void		DetermineTileSelection() override {};
		void		DrawOptions( VRAMViewer& oVRAMViewer ) override {};
};

class PaletteTab : public IEVRAMViewerOption
{
	public:
		void		UpdateTexture( VRAMViewer& oVRAMViewer )override {};
		void		DetermineTileSelection() override {};
		void		DrawOptions( VRAMViewer& oVRAMViewer ) override {};
};


#endif //BUCKYBUCK_VRAMVIEWER_H
