#version 450 core

layout(location = 0) out vec4 aFinalColor;

uniform samplerCube uTexture;

in vec3 vPosition;

void main()
{
	aFinalColor = textureLod(uTexture, vPosition, 0.0);
}