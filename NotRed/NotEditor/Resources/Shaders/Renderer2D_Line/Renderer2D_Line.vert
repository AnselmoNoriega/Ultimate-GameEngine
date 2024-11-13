#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjection;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} uRenderer;

layout (location = 0) out vec4 vColor;

void main()
{
	vColor = aColor;
	gl_Position = uViewProjection * uRenderer.Transform * vec4(aPosition, 1.0);
}