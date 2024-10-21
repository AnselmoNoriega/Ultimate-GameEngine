#version 450 core

layout(location = 0) out vec4 color;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
	float TexIndex;
	float TilingFactor;
};

layout (location = 0) in VertexOutput Input;

layout (binding = 1) uniform sampler2D uTextures[32];

void main()
{
	color = Input.Color;
}