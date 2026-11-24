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

constexpr int xSizeMiniature = 200;
constexpr int ySizeMiniature = 200;

static ImVec2 vSizeImage = ImVec2();

constexpr uint16_t TILE_DATA_ADDRESS_START = 0x8000;
constexpr uint16_t TILE_DATA_ADDRESS_END = 0x97FF;

constexpr uint16_t TILE_MAP_DATA_ADDRESS_START_1 = 0x9800;
constexpr uint16_t TILE_MAP_DATA_ADDRESS_END_1 = 0x9BFF;

constexpr uint16_t TILE_MAP_DATA_ADDRESS_START_2 = 0x9C00;
constexpr uint16_t TILE_MAP_DATA_ADDRESS_END_2 = 0x9FFF;

int			 IEVRAMViewerOption::m_iHoveredLine = 0;
int			 IEVRAMViewerOption::m_iHoveredCol = 0;

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


IEVRAMViewerOption::IEVRAMViewerOption() : IEVRAMViewerOption( 0,0,0,0,0 )
{
}

IEVRAMViewerOption::IEVRAMViewerOption( uint16_t XSize,uint16_t YSize,int TilesLine,int TilesCol,int SizeTile ) :
		m_iTexture( 0 ),
		m_iXSizeTexture( XSize ),
		m_iYSizeTexture( YSize ),
		m_iTilesPerLine( TilesLine ),
		m_iTilesPerCol( TilesCol ),
		m_iSizeTile( SizeTile ),
		m_fxOffsetGrid( 0.0f ),
		m_fyOffsetGrid( 0.0f ),
		m_bDrawGrid( true ),
		m_iAdressSelected( 0 )

{
}

void IEVRAMViewerOption::InitTexture()
{
	glGenTextures( 1,&m_iTexture );
	glBindTexture( GL_TEXTURE_2D,m_iTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_R8,m_iXSizeTexture,m_iYSizeTexture,0,GL_RED,GL_UNSIGNED_BYTE,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );

	glBindTexture( GL_TEXTURE_2D,0 );

	m_fxOffsetGrid = ( 1.0f / m_iTilesPerCol );
	m_fyOffsetGrid = ( 1.0f / m_iTilesPerLine );
}

void IEVRAMViewerOption::RenderTexture( VRAMViewer& oVRAMViewer )
{
	vSizeImage = ImVec2( ImGui::GetContentRegionAvail().x - XOffset,ImGui::GetContentRegionAvail().y );
	ImGui::BeginChild( "##TextureChild",vSizeImage,false,ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav );
	ImVec2 p = ImGui::GetCursorScreenPos();
	ImGui::Image( ( ImTextureID )( intptr_t )oVRAMViewer.GetFBOTexture(),vSizeImage,ImVec2( 0.0f,0.0f ),ImVec2( 1.0f,1.0f ) );
	ImGui::EndChild();

	if( m_bDrawGrid )
	{
		ImGui::BeginChild( "##TextureChild" );
		for( int i = 0; i <= m_iTilesPerCol; ++i )
		{
			float fOffset = ( vSizeImage.x / m_iTilesPerCol ) * i;
			ImGui::GetWindowDrawList()->AddLine( ImVec2( p.x + fOffset,p.y ),ImVec2( p.x + fOffset,p.y + vSizeImage.y ),ImGui::GetColorU32( ImGuiCol_Border ),2.0f );
		}
		for( int j = 0; j <= m_iTilesPerLine; ++j )
		{
			float fOffset = ( vSizeImage.y / m_iTilesPerLine ) * j;
			ImGui::GetWindowDrawList()->AddLine( ImVec2( p.x,p.y + fOffset ),ImVec2( p.x + vSizeImage.x,p.y + fOffset ),ImGui::GetColorU32( ImGuiCol_Border ),2.0f );
		}
		ImGui::EndChild();
	}

	ImGui::SameLine();
}

IEVRAMViewerOption::~IEVRAMViewerOption()
{
	glDeleteTextures( 1,&m_iTexture );
	m_iTexture = 0;
}

VRAMViewer::VRAMViewer() :
	m_iFBOTexture( 0 ),
	m_iVAO( 0 ),
	m_iVBO( 0 ),
	m_iEBO( 0 ),
	m_iFBO( 0 )
{

	_InitRenderer();
	_InitFramebuffer();

	glUseProgram( m_sShaderProgram.ID );
	GLint iColorPaletteLocation = glGetUniformLocation( m_sShaderProgram.ID,"colorPalette[0]" );
	float fGameBoyDMGPalette[ 16 ] =
	{
			0.675f, 0.831f, 0.565f, 1.0f,   // #ACD490
			0.910f, 0.988f, 0.800f, 1.0f,   // #E8FCCC
			0.329f, 0.549f, 0.439f, 1.0f,   // #548C70
			0.078f, 0.173f, 0.220f, 1.0f    // #142C38
	};

	glUniform4fv( iColorPaletteLocation,4,fGameBoyDMGPalette );

	m_aTabOption[ Tab::BGMAP ]		= std::make_unique<BGMapTab>();
	m_aTabOption[ Tab::TILES ]		= std::make_unique<TilesTab>();
	m_aTabOption[ Tab::OAM ]		= std::make_unique<OAMTab>();
	m_aTabOption[ Tab::PALETTE ]	= std::make_unique<PaletteTab>();
}

VRAMViewer::~VRAMViewer()
{
	glDeleteTextures( 1,&m_iFBOTexture );
	_DestroyRenderer();

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
		int8_t iCurrentTab = -1;
		if( ImGui::BeginTabBar("###TAB_BAR" ) )
		{
			if( ImGui::BeginTabItem( "BG Map" ) )
			{
				iCurrentTab = Tab::BGMAP;
				ImGui::EndTabItem();
			}
			if( ImGui::BeginTabItem( "Tiles" ) )
			{
				iCurrentTab = Tab::TILES;
				ImGui::EndTabItem();
			}
			if( ImGui::BeginTabItem( "OAM" ) )
			{
				iCurrentTab = Tab::OAM;
				ImGui::EndTabItem();
			}
			if( ImGui::BeginTabItem( "Palettes" ) )
			{
				iCurrentTab = Tab::PALETTE;
				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
		
		if( iCurrentTab != -1 )
		{
			m_aTabOption[ iCurrentTab ]->DetermineTileSelection();
			m_aTabOption[ iCurrentTab ]->UpdateTexture( *this );
			m_aTabOption[ iCurrentTab ]->RenderTexture( *this );
			m_aTabOption[ iCurrentTab ]->DrawOptions( *this );
		}
	}
	ImGui::End();

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();
}

void VRAMViewer::_InitFBOTexture()
{
	glGenTextures( 1,&m_iFBOTexture );
	glBindTexture( GL_TEXTURE_2D,m_iFBOTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_RGB,WidthScreen,HeightScreen,0,GL_RGB,GL_UNSIGNED_BYTE,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST );

	glBindTexture( GL_TEXTURE_2D,0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,m_iFBOTexture,0 );
}

void VRAMViewer::_InitFramebuffer()
{
	glGenFramebuffers( 1,&m_iFBO );
	glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );

	_InitFBOTexture();

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

void TilesTab::DetermineTileSelection()
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

		m_iHoveredCol = fRelativeX / ( vSizeImage.x / m_iTilesPerCol );
		m_iHoveredLine = fRelativeY / ( vSizeImage.y / m_iTilesPerLine );

		m_iAdressSelected = TILE_DATA_ADDRESS_START + ( m_iHoveredCol * m_iSizeTile ) + ( m_iHoveredLine * ( m_iSizeTile * m_iSizeTile ) );
		if( m_iAdressSelected < TILE_DATA_ADDRESS_START || m_iAdressSelected > TILE_DATA_ADDRESS_END )
			m_iAdressSelected = TILE_DATA_ADDRESS_START;
	}
}

void TilesTab::DrawOptions( VRAMViewer& oVRAMViewer )
{
	ImGui::BeginChild( "##Options",ImVec2( XOffset,vSizeImage.y ),false,ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav );

	float xStartCoord = m_iHoveredCol / static_cast< float >( m_iTilesPerCol );
	float yStartCoord = m_iHoveredLine / static_cast< float >( m_iTilesPerLine );
	ImGui::Image( ( ImTextureID )( intptr_t )oVRAMViewer.GetFBOTexture(),ImVec2( xSizeMiniature,ySizeMiniature ),ImVec2( xStartCoord,yStartCoord ),ImVec2( xStartCoord + m_fxOffsetGrid,yStartCoord + m_fyOffsetGrid ) );

	char aBuffer[ 6 ] = "";
	ImFormatString( aBuffer,sizeof( aBuffer ),"%02X",m_iAdressSelected );
	ImGui::PushItemWidth( ImGui::CalcTextSize( aBuffer ).x + 10.0f );
	ImGui::InputText( "##Adress",aBuffer,sizeof( aBuffer ),ImGuiInputTextFlags_ReadOnly );
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text( "Tile Adress" );

	ImGui::Checkbox( "Grid",&m_bDrawGrid );
	ImGui::EndChild();
}

void TilesTab::_SetTilesData()
{
	int iMem = TILE_DATA_ADDRESS_START;
	int posX = 0;
	int posY = 0;
	while( iMem < TILE_DATA_ADDRESS_END )
	{
		int pos = posX;
		for( int k = 0; k < 8; ++k )
		{
			uint8_t ilo = CPU::GetInstance()->ReadMemory( iMem );
			uint8_t ihi = CPU::GetInstance()->ReadMemory( iMem + 1 );

			for( int p = 0; p < 8; ++p )
			{
				int bit = 7 - p;
				m_iTilesDataPixels[ pos ][ posY + p ] = ( ( ( ihi >> bit ) & 1 ) << 1 ) | ( ( ilo >> bit ) & 1 );
			}

			pos++;
			iMem += 2;
		}

		posY += 8;
		posX += ( posY / m_iXSizeTexture ) * 8;
		posY %= m_iXSizeTexture;
	}
}

TilesTab::TilesTab() : IEVRAMViewerOption( 128, 192, 24, 16, 16 )
{
	memset( m_iTilesDataPixels,0,sizeof( m_iTilesDataPixels ) );
	InitTexture();
	m_iAdressSelected = TILE_DATA_ADDRESS_START;
}

void TilesTab::UpdateTexture( VRAMViewer& oVRAMViewer )
{
	_SetTilesData();

	glBindTexture( GL_TEXTURE_2D,m_iTexture );
	glBindFramebuffer( GL_FRAMEBUFFER,oVRAMViewer.GetFBOTexture() );
	glViewport( 0,0,WidthScreen,HeightScreen );

	glClearColor( 0.f,0.f,0.f,1.f );
	glClear( GL_COLOR_BUFFER_BIT );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D,m_iTexture );
	glTexSubImage2D( GL_TEXTURE_2D,0,0,0,m_iXSizeTexture,m_iYSizeTexture,GL_RED,GL_UNSIGNED_BYTE,m_iTilesDataPixels );
	oVRAMViewer.UseShader();
	glDrawElements( GL_TRIANGLES,6,GL_UNSIGNED_INT,0 );

	glBindFramebuffer( GL_FRAMEBUFFER,0 );
	glBindTexture( GL_TEXTURE_2D,0 );
	glViewport( 0,0,1920,1080 );
}

BGMapTab::BGMapTab() : IEVRAMViewerOption( 256, 256, 32, 32, 32 )
{
	memset( m_iBGMapDataPixels,0,sizeof( m_iBGMapDataPixels ) );
	InitTexture();
	m_iAdressSelected = TILE_MAP_DATA_ADDRESS_START_1;
}

void BGMapTab::UpdateTexture( VRAMViewer& oVRAMViewer )
{
	_SetTilesBGData();

	glBindTexture( GL_TEXTURE_2D,m_iTexture );
	glBindFramebuffer( GL_FRAMEBUFFER,oVRAMViewer.GetFBOTexture() );
	glViewport( 0,0,WidthScreen,HeightScreen );

	glClearColor( 0.f,0.f,0.f,1.f );
	glClear( GL_COLOR_BUFFER_BIT );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D,m_iTexture );
	glTexSubImage2D( GL_TEXTURE_2D,0,0,0,m_iXSizeTexture,m_iYSizeTexture,GL_RED,GL_UNSIGNED_BYTE,m_iBGMapDataPixels );
	oVRAMViewer.UseShader();
	glDrawElements( GL_TRIANGLES,6,GL_UNSIGNED_INT,0 );

	glBindFramebuffer( GL_FRAMEBUFFER,0 );
	glBindTexture( GL_TEXTURE_2D,0 );
	glViewport( 0,0,1920,1080 );
}

void BGMapTab::DetermineTileSelection()
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

		m_iHoveredCol = fRelativeX / ( vSizeImage.x / m_iTilesPerCol );
		m_iHoveredLine = fRelativeY / ( vSizeImage.y / m_iTilesPerLine );

		m_iAdressSelected = TILE_MAP_DATA_ADDRESS_START_1 + m_iHoveredCol + ( m_iHoveredLine * m_iSizeTile );
		if( m_iAdressSelected < TILE_MAP_DATA_ADDRESS_START_1 || m_iAdressSelected > TILE_MAP_DATA_ADDRESS_END_1 )
			m_iAdressSelected = TILE_MAP_DATA_ADDRESS_START_1;
	}
}

void BGMapTab::DrawOptions( VRAMViewer& oVRAMViewer )
{
	ImGui::BeginChild( "##Options",ImVec2( XOffset,vSizeImage.y ),false,ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav );

	float xStartCoord = m_iHoveredCol / static_cast< float >( m_iTilesPerCol );
	float yStartCoord = m_iHoveredLine / static_cast< float >( m_iTilesPerLine );
	ImGui::Image( ( ImTextureID )( intptr_t )oVRAMViewer.GetFBOTexture(),ImVec2( xSizeMiniature,ySizeMiniature ),ImVec2( xStartCoord,yStartCoord ),ImVec2( xStartCoord + m_fxOffsetGrid,yStartCoord + m_fyOffsetGrid ) );

	char aBuffer[ 6 ] = "";
	ImFormatString( aBuffer,sizeof( aBuffer ),"%02X",m_iAdressSelected );
	ImGui::PushItemWidth( ImGui::CalcTextSize( aBuffer ).x + 10.0f );
	ImGui::InputText( "##Adress",aBuffer,sizeof( aBuffer ),ImGuiInputTextFlags_ReadOnly );
	ImGui::PopItemWidth();
	ImGui::SameLine();
	ImGui::Text( "Map Adress" );

	ImGui::Checkbox( "Grid",&m_bDrawGrid );
	ImGui::EndChild();
}

void BGMapTab::_SetTilesBGData()
{
	int iMem = TILE_MAP_DATA_ADDRESS_START_1;
	int posX = 0;
	int posY = 0;
	while( iMem < TILE_MAP_DATA_ADDRESS_END_1 )
	{
		int pos = posX;

		uint16_t iAdress = CPU::GetInstance()->ReadMemory( iMem );
		iAdress = ( iAdress << 4 ) | 0x9000;
		if( iAdress > 0x97F0 )
			iAdress = ( CPU::GetInstance()->ReadMemory( iMem ) << 4 ) | 0x8000;
		
		for( int k = 0; k < 8; ++k )
		{
			uint8_t ilo = CPU::GetInstance()->ReadMemory( iAdress );
			uint8_t ihi = CPU::GetInstance()->ReadMemory( iAdress + 1 );

			for( int p = 0; p < 8; ++p )
			{
				int bit = 7 - p;
				m_iBGMapDataPixels[ pos ][ posY + p ] = ( ( ( ihi >> bit ) & 1 ) << 1 ) | ( ( ilo >> bit ) & 1 );
			}

			pos++;
			iAdress += 2;
		}

		posY += 8;
		posX += ( posY / m_iXSizeTexture ) * 8;
		posY %= m_iXSizeTexture;
		iMem ++;
	}
}
