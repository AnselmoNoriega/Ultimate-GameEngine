#version 450 core

layout(location = 0) out vec4 finalColor;
layout(location = 1) out vec4 oBloom;
layout(location = 2) out vec4 unuesd;

layout (binding = 1) uniform samplerCube uTexture;

#ifdef OPENGL

layout (std140, binding = 13) uniform Uniforms 
{
	float TextureLod;
	float Intensity;
} uUniforms;

#else

layout (push_constant) uniform Uniforms
{
	float TextureLod;
	float Intensity;
} uUniforms;

#endif

layout (location = 0) in vec3 vPosition;

void main()
{
	finalColor = textureLod(uTexture, vPosition, uUniforms.TextureLod);
	finalColor.rgb = finalColor.rgb * uUniforms.Intensity;
	oBloom = vec4(0.0);
	unuesd = vec4(0.0);
}