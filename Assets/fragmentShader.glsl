#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D oTexture;
uniform vec4 colorPalette[ 4 ];

void main()
{
	float v = texture( oTexture, TexCoord ).r;
	int idx = int( v * 255.0 + 0.5);

	vec3 color = colorPalette[ idx ].rgb;
	FragColor = vec4( color, 1.0 );
}
