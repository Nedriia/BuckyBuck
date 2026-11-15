//
// Created by arthu on 05/11/2026.
//

#include "VRAMViewer.h"
#include <chrono>
#include "imgui.h"
#include <imgui_internal.h>
#include "CPU.h"
#include "Display.h"

constexpr int XOffset = 250;

constexpr int WidthScreen = 768;
constexpr int HeightScreen = 1152;

constexpr int XSizeTexture = 128;
constexpr int YSizeTexture = 192;

constexpr int xSizeMiniature = 200;
constexpr int ySizeMiniature = 200;

constexpr int TilesPerLine = 24;
constexpr int TilesPerCol = 16;
constexpr int SizeTile = 16;

constexpr float xOffsetGrid = ( 1 / 16.0f );
constexpr float yOffsetGrid = ( 1 / 24.0f );

static ImVec2 vSizeImage = ImVec2();

constexpr uint16_t START_VRAM = 0x8000;
constexpr uint16_t END_VRAM = 0x97FF;

unsigned int VRAMViewer::m_iVRAMTexture = 0;
unsigned int VRAMViewer::m_iFBOTexture = 0;
uint16_t	 VRAMViewer::m_iAdressSelected = START_VRAM;
int			 VRAMViewer::m_iHoveredLine = 0;
int			 VRAMViewer::m_iHoveredCol = 0;

float vertices[] = {
	// positions		//Texture coords
	1.0f, 1.0f, 0.0f,	1.0f, 1.0f,		// top right
	1.0f, -1.0f, 0.0f,	1.0f, 0.0f,		// bottom right
	-1.0f, -1.0f, 0.0f,	0.0f, 0.0f,		// bottom left 
	-1.0f, 1.0f, 0.0f,	0.0f, 1.0f,		// top left
};

unsigned int indices[] = {
	0, 1, 3,
	1, 2, 3
};

VRAMViewer::VRAMViewer() :
	m_iVAO( 0 ),
	m_iVBO( 0 ),
	m_iEBO( 0 ),
	m_iFBO( 0 ),
	m_bDrawGrid( true )
{
	memset( m_iVRAMPixels,0,sizeof( m_iVRAMPixels ) );

	_InitRenderer();
	_InitFramebuffer();

	glUseProgram( m_sShaderProgram.ID );
	GLint iColorPaletteLocation = glGetUniformLocation( m_sShaderProgram.ID,"colorPalette[0]" );
	float fGameBoyDMGPalette[ 16 ] =
	{
		0.878f, 0.973f, 0.816f, 1.0f,
		0.533f, 0.753f, 0.439f, 1.0f,
		0.204f, 0.408f, 0.337f, 1.0f,
		0.031f, 0.094f, 0.125f, 1.0f
	};

	glUniform4fv( iColorPaletteLocation,4,fGameBoyDMGPalette );
}

VRAMViewer::~VRAMViewer()
{
	glDeleteTextures( 1,&m_iVRAMTexture );
	glDeleteTextures( 1,&m_iFBOTexture );
	_DestroyRenderer();

	m_iVRAMTexture = 0;
	m_iFBOTexture = 0;
	m_iVAO = 0;
	m_iVBO = 0;
	m_iEBO = 0;
	m_iFBO = 0;

	m_sShaderProgram.Delete();
}

void VRAMViewer::Update()
{
	auto start = std::chrono::high_resolution_clock::now();

	glfwPollEvents();
	static double iDurationMs;

	char titleBuffer[ 64 ];
	std::snprintf( titleBuffer,sizeof( titleBuffer ),"VRAM Viewer (%.2f ms)###VRAMDisplayWindow",iDurationMs );
	if( ImGui::Begin( titleBuffer,nullptr ) )
	{
		_DetermineTileSelection();
		_UpdateTexture();
		_DisplayTexture();
	}
	ImGui::End();

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
}

void VRAMViewer::_InitTexture()
{
	glGenTextures( 1,&m_iFBOTexture );
	glBindTexture( GL_TEXTURE_2D,m_iFBOTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_RGB,WidthScreen,HeightScreen,0,GL_RGB,GL_UNSIGNED_BYTE,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );

	glBindTexture( GL_TEXTURE_2D,0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,m_iFBOTexture,0 );

	glGenTextures( 1,&m_iVRAMTexture );
	glBindTexture( GL_TEXTURE_2D,m_iVRAMTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_R8,XSizeTexture,YSizeTexture,0,GL_RED,GL_UNSIGNED_BYTE,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );

	//glBindTexture( GL_TEXTURE_2D,0 );
}

void VRAMViewer::_InitFramebuffer()
{
	glGenFramebuffers( 1,&m_iFBO );
	glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );

	_InitTexture();

	glBindFramebuffer( GL_FRAMEBUFFER,0 );

	if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
		std::cerr << "DISPLAY::FRAMEBUFFER_NOT_COMPLETE" << std::endl;
}

void VRAMViewer::_InitRenderer()
{
	glGenVertexArrays( 1,&m_iVAO );
	glBindVertexArray( m_iVAO ); //Need to be bind before, any vertex attributs call will be stored inside the VAO
	glGenBuffers( 1,&m_iVBO );
	glGenBuffers( 1,&m_iEBO );

	glBindBuffer( GL_ARRAY_BUFFER,m_iVBO );
	glBufferData( GL_ARRAY_BUFFER,sizeof( vertices ),vertices,GL_STATIC_DRAW );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER,m_iEBO );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER,sizeof( indices ),indices,GL_STATIC_DRAW );

	glVertexAttribPointer( 0,3,GL_FLOAT,GL_FALSE,5 * sizeof( float ),( void* )0 ); //Vertices
	glEnableVertexAttribArray( 0 );

	glVertexAttribPointer( 1,2,GL_FLOAT,GL_FALSE,5 * sizeof( float ),( void* )( 3 * sizeof( float ) ) ); //UV coords
	glEnableVertexAttribArray( 1 );

	glBindVertexArray( m_iVAO );

	m_sShaderProgram = Shader( "vertexShader.glsl","fragmentShader.glsl" );
}

void VRAMViewer::_DestroyRenderer()
{
	glDeleteVertexArrays( 1,&m_iVAO );
	glDeleteBuffers( 1,&m_iVBO );
	glDeleteBuffers( 1,&m_iEBO );
	glDeleteFramebuffers( 1,&m_iFBO );
}

void VRAMViewer::_GetVRAMData()
{
	int iMem = START_VRAM;
	int posX = 0;
	int posY = 0;
	while( iMem < END_VRAM )
	{
		int pos = posX;
		for( int k = 0; k < 8; ++k )
		{
			uint8_t ilo = CPU::GetInstance()->ReadMemory( iMem );
			uint8_t ihi = CPU::GetInstance()->ReadMemory( iMem + 1 );

			for( int p = 0; p < 8; ++p )
			{
				int bit = 7 - p;
				m_iVRAMPixels[ pos ][ posY + p ] = ( ( ( ihi >> bit ) & 1 ) << 1 ) | ( ( ilo >> bit ) & 1 );
			}

			pos++;
			iMem += 2;
		}

		posY += 8;
		posX += ( posY / XSizeTexture ) * 8;
		posY %= XSizeTexture;
	}
}

void VRAMViewer::_UpdateTexture()
{
	_GetVRAMData();

	glBindTexture( GL_TEXTURE_2D,m_iVRAMTexture );
	glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );
	glViewport( 0,0,WidthScreen,HeightScreen );

	glClearColor( 0.f,0.f,0.f,1.f );
	glClear( GL_COLOR_BUFFER_BIT );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D,m_iVRAMTexture );
	glTexSubImage2D( GL_TEXTURE_2D,0,0,0,XSizeTexture,YSizeTexture,GL_RED,GL_UNSIGNED_BYTE,m_iVRAMPixels );
	m_sShaderProgram.Use();
	glDrawElements( GL_TRIANGLES,6,GL_UNSIGNED_INT,0 );

	glBindFramebuffer( GL_FRAMEBUFFER,0 );
	glBindTexture( GL_TEXTURE_2D,0 );
	glViewport( 0,0,1920,1080 );
}

void VRAMViewer::_DisplayTexture()
{
	vSizeImage = ImVec2( ImGui::GetContentRegionAvail().x - XOffset,ImGui::GetContentRegionAvail().y );
	ImGui::BeginChild( "##TextureChild",vSizeImage,false,ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav );
	ImVec2 p = ImGui::GetCursorScreenPos();
	ImGui::Image( ( ImTextureID )( intptr_t )m_iFBOTexture,vSizeImage,ImVec2( 0.0f,0.0f ),ImVec2( 1.0f,1.0f ) );
	ImGui::EndChild();

	if( m_bDrawGrid )
	{
		ImGui::BeginChild( "##TextureChild" );
		for( int i = 0; i <= TilesPerCol; ++i )
		{
			float fOffset = ( vSizeImage.x / TilesPerCol ) * i;
			ImGui::GetWindowDrawList()->AddLine( ImVec2( p.x + fOffset,p.y ),ImVec2( p.x + fOffset,p.y + vSizeImage.y ),ImGui::GetColorU32( ImGuiCol_Border ),2.0f );
		}
		for( int j = 0; j <= TilesPerLine; ++j )
		{
			float fOffset = ( vSizeImage.y / TilesPerLine ) * j;
			ImGui::GetWindowDrawList()->AddLine( ImVec2( p.x,p.y + fOffset ),ImVec2( p.x + vSizeImage.x,p.y + fOffset ),ImGui::GetColorU32( ImGuiCol_Border ),2.0f );
		}
		ImGui::EndChild();
	}

	ImGui::SameLine();
	ImGui::BeginChild( "##Options",ImVec2( XOffset,vSizeImage.y ),false,ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav );
	
	float xStartCoord = m_iHoveredCol / static_cast< float >( TilesPerCol );
	float yStartCoord = m_iHoveredLine / static_cast< float >( TilesPerLine );
	ImGui::Image( ( ImTextureID )( intptr_t )m_iFBOTexture,ImVec2( xSizeMiniature,ySizeMiniature ),ImVec2( xStartCoord,yStartCoord ),ImVec2( xStartCoord + xOffsetGrid,yStartCoord + yOffsetGrid ) );

	char aBuffer[ 6 ] = "";
	ImFormatString( aBuffer,sizeof( aBuffer ),"%02X",m_iAdressSelected );
	ImGui::PushItemWidth( ImGui::CalcTextSize( aBuffer ).x + 10.0f );
	ImGui::InputText( "##Adress",aBuffer,sizeof( aBuffer ),ImGuiInputTextFlags_ReadOnly );
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text( "Tile Adress" );

	ImGui::Checkbox("Grid", &m_bDrawGrid );
	ImGui::EndChild();
}

void VRAMViewer::_DetermineTileSelection()
{
	ImVec2 vCursorPos = ImGui::GetCursorScreenPos();

	ImVec2 zone_min(
		vCursorPos.x,
		vCursorPos.y
	);

	ImVec2 zone_max(
		vCursorPos.x + vSizeImage.x,
		vCursorPos.y + vSizeImage.y );

	if( ImGui::IsMouseHoveringRect( zone_min,zone_max ) )
	{
		ImVec2 mouse_pos = ImGui::GetMousePos();
		float fRelativeX = ( mouse_pos.x - vCursorPos.x );
		float fRelativeY = ( mouse_pos.y - vCursorPos.y );

		m_iHoveredCol = fRelativeX / ( vSizeImage.x / TilesPerCol );
		m_iHoveredLine = fRelativeY / ( vSizeImage.y / TilesPerLine );

		m_iAdressSelected = START_VRAM + ( m_iHoveredCol * SizeTile ) + ( m_iHoveredLine * ( SizeTile * SizeTile ) );
		if( m_iAdressSelected < START_VRAM || m_iAdressSelected > END_VRAM )
			m_iAdressSelected = START_VRAM;
	}
}
