#version 450 core

layout(location = 0) in vec3 aPosition;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjection;
};

layout (std140, binding = 1) uniform Transform
{
	mat4 uTransform;
};

void main()
{
	gl_Position = uViewProjection * uTransform * vec4(aPosition, 1.0);
}