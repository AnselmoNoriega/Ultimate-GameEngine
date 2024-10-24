#version 450

layout(binding = 0)  uniform sampler2DArray uTexResultsArray;
layout(location = 0) out vec4 out_Color;

//----------------------------------------------------------------------------------

void main() 
{
	ivec2 fullResPos = ivec2(gl_FragCoord.xy);
	ivec2 offset = fullResPos & 3;
	int sliceId = offset.y * 4 + offset.x;
	ivec2 quarterResPos = fullResPos >> 2;

	out_Color = vec4(texelFetch(uTexResultsArray, ivec3(quarterResPos, sliceId), 0).xy, 0, 1.0);
}