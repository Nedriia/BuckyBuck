//
// Created by arthu on 05/11/2026.
//

#include "VRAMViewer.h"
#include <chrono>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "CPU.h"
#include "Display.h"

unsigned int VRAMViewer::m_iVRAMTexture = 0;
unsigned int VRAMViewer::m_iFBOTexture = 0;

#define START_VRAM  0x8000
#define END_VRAM  0x97FF

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
	m_iFBO( 0 )
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

	char titleBuffer[ 128 ];
	std::snprintf( titleBuffer,sizeof( titleBuffer ),"VRAM Viewer (%.2f ms)###VRAMDisplayWindow",iDurationMs );
	if( ImGui::Begin( titleBuffer,nullptr ) )
	{
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

	glTexImage2D( GL_TEXTURE_2D,0,GL_RGB,1920,1080,0,GL_RGB,GL_UNSIGNED_BYTE,NULL );

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR );

	glBindTexture( GL_TEXTURE_2D,0 );
	glFramebufferTexture2D( GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,m_iFBOTexture,0 );


	glGenTextures( 1,&m_iVRAMTexture );
	glBindTexture( GL_TEXTURE_2D,m_iVRAMTexture );

	glTexImage2D( GL_TEXTURE_2D,0,GL_R8,128,192,0,GL_RED,GL_UNSIGNED_BYTE,NULL );

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
		posX += ( posY / 128 ) * 8;
		posY %= 128;
	}
}

void VRAMViewer::_UpdateTexture()
{
	_GetVRAMData();

	glClearColor( 0.f,0.f,0.f,1.f );
	glClear( GL_COLOR_BUFFER_BIT );

	glBindFramebuffer( GL_FRAMEBUFFER,m_iFBO );
	glTexSubImage2D( GL_TEXTURE_2D,0,0,0,128,192,GL_RED,GL_UNSIGNED_BYTE,m_iVRAMPixels );

	m_sShaderProgram.Use();

	glDrawElements( GL_TRIANGLES,6,GL_UNSIGNED_INT,0 );
	glBindFramebuffer( GL_FRAMEBUFFER,0 );
}

void VRAMViewer::_DisplayTexture()
{
	ImVec2 avail = ImVec2( ImGui::GetContentRegionAvail().x,ImGui::GetContentRegionAvail().y - 14 );
	ImGui::Image( ( ImTextureID )( intptr_t )m_iFBOTexture,avail,ImVec2( 0.0f,0.0f ),ImVec2( 1.0f,1.0f ) );
}