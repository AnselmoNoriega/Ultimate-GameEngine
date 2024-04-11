#version 450 core

layout(location = 0) out vec4 oColor;

struct VertexOutput
{
	vec4 Color;
};

layout (location = 0) in VertexOutput Input;

void main()
{
	oColor = Input.Color;
}