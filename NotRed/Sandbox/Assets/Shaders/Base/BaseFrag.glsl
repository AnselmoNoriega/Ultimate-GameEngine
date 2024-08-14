#version 450 core

layout(location = 0) out vec4 finalColor;

uniform vec4 uColor;

void main()
{
	finalColor = vec4(uColor);
}