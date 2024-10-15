#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 unused;

layout (push_constant) uniform Material
{
	layout (offset = 64) vec4 Color;
} uMaterialUniforms;

void main()
{
	color = uMaterialUniforms.Color;
	unused = vec4(0.0);
}