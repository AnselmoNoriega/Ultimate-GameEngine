#version 450 core

layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec4 oBloom;

layout (binding = 1) uniform samplerCube uTexture;

layout (push_constant) uniform Uniforms
{
	float TextureLod;
} uUniforms;

layout (location = 0) in vec3 vPosition;

void main()
{
	finalColor = textureLod(uTexture, vPosition, uUniforms.TextureLod);
	oBloom = vec4(0.0);
}