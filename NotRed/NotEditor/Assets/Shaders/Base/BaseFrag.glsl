#version 450 core

layout(location = 0) out vec4 finalColor;

uniform vec4 uColor;

in vec3 vNormal;

void main()
{
	finalColor = vec4((vNormal * 0.5 + 0.5), 1.0);// * u_Color.xyz, 1.0);
}