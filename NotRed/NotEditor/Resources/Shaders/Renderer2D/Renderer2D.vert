#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in float aTexIndex;
layout(location = 4) in float aTilingFactor;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjection;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} uRenderer;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) out VertexOutput Output;
layout (location = 5) out flat float TexIndex;

void main()
{
	Output.Color = aColor;
	Output.TexCoord = aTexCoord;
	TexIndex = aTexIndex;
	Output.TilingFactor = aTilingFactor;
	gl_Position = uViewProjection * uRenderer.Transform * vec4(aPosition, 1.0);
}