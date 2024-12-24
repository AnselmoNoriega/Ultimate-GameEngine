#version 450 core

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 unused;
layout(location = 2) out vec4 unused2;

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

float Convert_sRGB_FromLinear(float theLinearValue)
{
	return theLinearValue <= 0.0031308f
		? theLinearValue * 12.92f
		: pow(theLinearValue, 1.0f / 2.4f) * 1.055f - 0.055f;
}

void main()
{
    oColor = textureLod(uTexture, vPosition, uUniforms.TextureLod) * uUniforms.Intensity;
    unused = vec4(0);
    unused2 = vec4(0);
}