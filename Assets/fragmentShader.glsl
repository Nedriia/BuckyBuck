#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D oTexture;
vec3 bgColor = vec3( 0.20,0.40,0.33 );
vec3 fontColor = vec3( 0.87,0.97,0.81 );

void main()
{
	float v = texture( oTexture, TexCoord ).r;
	vec3 color = mix( bgColor, fontColor, v );
	FragColor = vec4( color, 1.0 );
}
