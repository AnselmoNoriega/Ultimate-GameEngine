#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

layout(std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
	mat4 uInverseViewProjectionMatrix;
	mat4 uProjectionMatrix;
	mat4 uViewMatrix;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} uRenderer;

layout(location = 0) out float LinearDepth;

void main()
{
	vec4 worldPosition = uRenderer.Transform * vec4(aPosition, 1.0);
	LinearDepth = -(uViewMatrix * worldPosition).z;
	gl_Position = uViewProjectionMatrix * worldPosition;
}