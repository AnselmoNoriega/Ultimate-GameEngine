#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

layout (std140, binding = 1) uniform ShadowData
{
	mat4 uViewProjectionMatrix;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} uRenderer;

void main()
{
	gl_Position = uViewProjectionMatrix * uRenderer.Transform * vec4(aPosition, 1.0);
}