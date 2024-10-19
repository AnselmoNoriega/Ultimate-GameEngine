#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) out OutputBlock Output;

void main()
{
	vec4 position = vec4(aPosition.xy, 0.0, 1.0);
	Output.TexCoord = aTexCoord;
	gl_Position = position;
}