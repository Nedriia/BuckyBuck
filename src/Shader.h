//
// Created by arthu on 05/11/2026.
//

#pragma once

#ifndef BUCKYBUCK_SHADER_H
#define BUCKYBUCK_SHADER_H

#include <glad/glad.h>

class Shader
{
public:
	Shader();
	Shader( const char* VertexPath,const char* FragmentPath );
	void Use();
	void Delete();

	unsigned int ID;
};

#endif //BUCKYBUCK_SHADER_H
