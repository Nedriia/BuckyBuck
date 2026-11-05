//
// Created by arthu on 05/11/2026.
//

#include "Shader.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader()
{
	ID = 0;
}

Shader::Shader( const char* VertexPath,const char* FragmentPath )
{
	ID = 0;

	std::string vertexCode;
	std::string fragmentCode;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;

	vShaderFile.exceptions( std::ifstream::failbit | std::ifstream::badbit );
	fShaderFile.exceptions( std::ifstream::failbit | std::ifstream::badbit );
	try
	{
		vShaderFile.open( std::string( PATH_SHADERS ) + VertexPath );
		fShaderFile.open( std::string( PATH_SHADERS ) + FragmentPath );
		std::stringstream vShaderStream,fShaderStream;
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		vShaderFile.close();
		fShaderFile.close();
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch( std::ifstream::failure e )
	{
		std::cerr << "ERROR::SHADER::FILE_FAILED_ON_READ" << std::endl;
	}

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	unsigned int vertexShader,fragmentShader;
	vertexShader = glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( vertexShader,1,&vShaderCode,nullptr );
	glCompileShader( vertexShader );

	int success;
	char infolog[ 512 ];

	glGetShaderiv( vertexShader,GL_COMPILE_STATUS,&success );
	if( !success )
	{
		glGetShaderInfoLog( vertexShader,512,nullptr,infolog );
		std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infolog << std::endl;
	}

	fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
	glShaderSource( fragmentShader,1,&fShaderCode,nullptr );
	glCompileShader( fragmentShader );
	glGetShaderiv( fragmentShader,GL_COMPILE_STATUS,&success );
	if( !success )
	{
		glGetShaderInfoLog( fragmentShader,512,nullptr,infolog );
		std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infolog << std::endl;
	}

	//Shader Program
	ID = glCreateProgram();
	glAttachShader( ID,vertexShader );
	glAttachShader( ID,fragmentShader );
	glLinkProgram( ID );
	glGetProgramiv( ID,GL_LINK_STATUS,&success );
	if( !success )
	{
		glGetProgramInfoLog( ID,512,nullptr,infolog );
		std::cerr << "ERROR::SHADER::PROGRAM::LINK_FAILED\n" << infolog << std::endl;
	}

	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ); //Wireframe
	glDeleteShader( vertexShader );
	glDeleteShader( fragmentShader );
}

void Shader::Use()
{
	glUseProgram( ID );
}

void Shader::Delete()
{
	glDeleteProgram( ID );
}