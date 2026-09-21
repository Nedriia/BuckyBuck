//
// Created by arthu on 18/09/2026.
//
#pragma once
#ifndef BUCKYBUCK_DISPLAY_H
#define BUCKYBUCK_DISPLAY_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define DEBUG_INFO

class Display
{
public:
	Display();
	~Display();

	int Init();
	int CreateMainWindow();
	void Update( bool &quit );
	void DestroyWindow();

	static Display* GetInstance()
	{
		if( m_pSingleton == nullptr )
			m_pSingleton = new Display;
		return m_pSingleton;
	}

private:
	static void framebuffer_size_callback( GLFWwindow* m_pWindow,int width,int height );

	GLFWwindow*							m_pWindow;
	static Display*						m_pSingleton;

};


#endif //BUCKYBUCK_DISPLAY_H
