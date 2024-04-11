#version 450 core

layout(location = 0) in vec3 aWorldPosition;
layout(location = 1) in vec3 aLocalPosition;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aThickness;
layout(location = 5) in int aEntityID;

layout(std140, binding = 0) uniform Camera
{
	mat4 uViewProjection;
};

struct VertexOutput
{
	vec3 LocalPosition;
	vec4 Color;
	float Thickness;
};

layout (location = 0) out VertexOutput Output;
layout (location = 3) out flat int vEntityID;

void main()
{
	Output.LocalPosition = aLocalPosition;
	Output.Color = aColor;
	Output.Thickness = aThickness;

	vEntityID = aEntityID;

	gl_Position = uViewProjection * vec4(aWorldPosition, 1.0);
}