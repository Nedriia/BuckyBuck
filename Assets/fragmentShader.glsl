#version 330 core

out vec4 FragColor;
in vec2 TexCoord;

uniform usampler2DArray oTexture;

uniform vec4 colorPalette[ 4 ];

uniform int Width;
uniform int Height;

void main()
{
	vec2 oCoords = vec2( int( TexCoord.x * Width ), int ( TexCoord.y * Height ) );	//Convert texture ratio to coords
	int iBlock = int( oCoords.x ) / 32;												//Which block are we, texture sent as X block of 32 bits
	uint iIndex = uint ( oCoords.x ) % 32u;											//Look for the pixel

	uvec4 texel0 = texelFetch( oTexture, ivec3( iBlock, oCoords.y, 0 ), 0 );		//Extract memory block
	uint v0 = ( texel0.r >> iIndex ) & 1u;

	uvec4 texel1 = texelFetch( oTexture, ivec3( iBlock, oCoords.y, 1 ), 0 );
	uint v1 = ( texel1.r >> iIndex ) & 1u;

	uint colorIndex = ( v1 << 1u ) | v0;

	vec3 color = colorPalette[ colorIndex ].rgb;
	FragColor = vec4(color, 1.0);
}
