//
// Created by arthu on 18/09/2026.
//

#include "Display.h"

#include <chrono>
#include <iostream>
#include "DebugInfosDisplay.h"
#include "imgui.h"

// settings
const uint16_t WINDOW_WIDTH = 1920;
const uint16_t WINDOW_HEIGHT = 1080;

#ifdef LEAK_DETECTOR
#include <vld.h>
#define ENABLE_GLOBAL_LEAK_DETECTION() VLDGlobalEnable()
#define DISABLE_GLOBAL_LEAK_DETECTION() VLDGlobalDisable()

#define ENABLE_SPECIFIC_LEAK_DETECTION() VLDEnable()
#define DISABLE_SPECIFIC_LEAK_DETECTION() VLDDisable()
#else
#define ENABLE_GLOBAL_LEAK_DETECTION() ((void)0)
#define DISABLE_GLOBAL_LEAK_DETECTION() ((void)0)

#define ENABLE_SPECIFIC_LEAK_DETECTION() ((void)0)
#define DISABLE_SPECIFIC_LEAK_DETECTION() ((void)0)
#endif

Display* Display::m_pSingleton = nullptr;

Display::Display() :
	m_pWindow( nullptr )
{

}

Display::~Display()
{
	m_pSingleton = nullptr;
}

static void glfw_error_callback( int error,const char* description )
{
	fprintf( stderr,"GLFW Error %d: %s\n",error,description );
}

int Display::Init()
{
	glfwSetErrorCallback( glfw_error_callback );
	if( !glfwInit() )
		return -1;

	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR,3 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR,3 );
	glfwWindowHint( GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE );

	if ( CreateMainWindow() != 0 )
		return -1;

#ifdef DEBUG_INFO
	DebugInfosDisplay::GetInstance()->Init( m_pWindow );
#endif

	return 0;
}


int Display::CreateMainWindow()
{
	// glfw window creation
	// --------------------
	DISABLE_SPECIFIC_LEAK_DETECTION();
	m_pWindow = glfwCreateWindow( WINDOW_WIDTH,WINDOW_HEIGHT,"Bucky Buck",nullptr,nullptr );
	if( m_pWindow == nullptr )
	{
		std::cerr << "DISPLAY::FAILED_TO_CREATE_GLFW_WINDOW" << std::endl;
		return -1;
	}
	ENABLE_SPECIFIC_LEAK_DETECTION();

	glfwMakeContextCurrent( m_pWindow );
	glfwSetFramebufferSizeCallback( m_pWindow,Display::framebuffer_size_callback );

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if( !gladLoadGLLoader( ( GLADloadproc )glfwGetProcAddress ) )
	{
		std::cerr << "DISPLAY::GLAD_FAILED_TO_INIT" << std::endl;
		return -1;
	}

	glfwSwapInterval( 1 ); //Put 0 in case you want to uncap the speed
	return 0;
}

void Display::Update( bool &quit )
{
	if( m_pWindow == nullptr )
		return;

	if( !glfwWindowShouldClose( m_pWindow ) )
	{
		if( glfwGetKey( m_pWindow,GLFW_KEY_ESCAPE ) == GLFW_PRESS )
			glfwSetWindowShouldClose( m_pWindow,true );

		glfwPollEvents();
	}
	else
	{
		quit = true;
		return;
	}

	auto start = std::chrono::high_resolution_clock::now();

#ifdef DEBUG_INFO
	DebugInfosDisplay::GetInstance()->StartFrame();
	DebugInfosDisplay::GetInstance()->Update();
	DebugInfosDisplay::GetInstance()->Render();

	glfwSwapBuffers( m_pWindow );
#endif

	static double iDurationMs;
	char titleBuffer[ 128 ];
	std::snprintf( titleBuffer,sizeof( titleBuffer ),"Bucky Buck (%.2f ms)",iDurationMs );

	auto end = std::chrono::high_resolution_clock::now();
	iDurationMs = std::chrono::duration<double,std::milli>( end - start ).count();

	glfwSetWindowTitle( m_pWindow,titleBuffer );
}

void Display::DestroyWindow()
{
#ifdef DEBUG_INFO
	DebugInfosDisplay::GetInstance()->Destroy();
#endif

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	if ( m_pWindow )
		glfwDestroyWindow( m_pWindow );
	glfwTerminate();

	m_pWindow = nullptr;
	delete m_pSingleton;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void Display::framebuffer_size_callback( GLFWwindow* m_pWindow,int width,int height )
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport( 0,0,width,height );
}