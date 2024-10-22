#version 450 core

layout(location = 0) in vec3 aWorldPosition;
layout(location = 1) in float aThickness;
layout(location = 2) in vec2 aLocalPosition;
layout(location = 3) in vec4 aColor;

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
	vec2 LocalPosition;
	float Thickness;
	vec4 Color;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	Output.LocalPosition = aLocalPosition;
	Output.Thickness = aThickness;
	Output.Color = aColor;
	gl_Position = uViewProjection * uRenderer.Transform * vec4(aWorldPosition, 1.0);
}