#version 450 core

layout(location = 0) out vec4 finalColor;

uniform samplerCube uTexture;
uniform float uTextureLod;
uniform float uSkyIntensity;

in vec3 vPosition;

void main()
{
	vec3 color = textureLod(uTexture, vPosition, uTextureLod).rgb * uSkyIntensity;
	finalColor =  vec4(color, 1.0);
}