//
// Created by arthu on 18/09/2026.
//

#include "DebugInfosDisplay.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

std::unique_ptr<HexEditor_ImGUI>			DebugInfosDisplay::m_oHexEditor;
DebugInfosDisplay*							DebugInfosDisplay::m_pSingleton = nullptr;

DebugInfosDisplay::DebugInfosDisplay() :
	m_pWindow( nullptr )
{}

DebugInfosDisplay::~DebugInfosDisplay()
{
	m_pSingleton = nullptr;
}

void DebugInfosDisplay::Init( GLFWwindow* mainWindow )
{
	if ( mainWindow )
		m_pWindow = mainWindow;
	else
	{
		std::cerr << "WINDOW FROM DISPLAY NULL, CAN'T CONTINUE" << std::endl;
		return;
	}

	float main_scale = ImGui_ImplGlfw_GetContentScaleForMonitor( glfwGetPrimaryMonitor() ); // Valid on GLFW 3.3+ only

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup Dear ImGui style
	ImGui::StyleColorsClassic();
	//ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes( main_scale );        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL( m_pWindow,true );
#ifdef __EMSCRIPTEN__
	ImGui_ImplGlfw_InstallEmscriptenCallbacks( m_pWindow,"#canvas" );
#endif
	ImGui_ImplOpenGL3_Init( "#version 330" );

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	m_oHexEditor = std::make_unique<HexEditor_ImGUI>();
	m_oHexEditor->Init( m_pWindow );
}

void DebugInfosDisplay::StartFrame()
{
	glfwPollEvents();
	if( glfwGetWindowAttrib( m_pWindow,GLFW_ICONIFIED ) != 0 )
	{
		ImGui_ImplGlfw_Sleep( 10 );
		return;
	}

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::DockSpaceOverViewport();
}

void DebugInfosDisplay::Render()
{
	int display_w,display_h;
	glfwGetFramebufferSize( m_pWindow,&display_w,&display_h );
	glViewport( 0,0,display_w,display_h );

	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}

void DebugInfosDisplay::Update()
{
	m_oHexEditor->Update();
}

void DebugInfosDisplay::Destroy()
{
	if ( m_pWindow != nullptr )
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext();
	}
	delete m_pSingleton;
}
