#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoords;

struct VertexOutput
{
	vec2 TexCoords;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	Output.TexCoords = aTexCoords;
	gl_Position = vec4(aPosition, 1.0f);
}