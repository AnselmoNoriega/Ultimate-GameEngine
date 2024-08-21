#version 450 core

layout(location = 0) out vec4 finalColor;

uniform samplerCube uTexture;
uniform float uTextureLod;

in vec3 vPosition;

void main()
{
	finalColor = textureLod(uTexture, vPosition, uTextureLod);
}