#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

layout(binding = 0) uniform Camera
{
    mat4 uViewProjectionMatrix;
    mat4 uInverseViewProjectionMatrix;
    mat4 uProjectionMatrix;
    mat4 uInverseProjectionMatrix;
    mat4 uViewMatrix;
    mat4 uInverseViewMatrix;
    mat4 uFlippedViewProjectionMatrix;
};

layout (pushconstant) uniform Transform
{
	mat4 Transform;
} uRenderer;

layout(location = 0) out float LinearDepth;

void main()
{
    vec4 worldPosition = uRenderer.Transform * vec4(aPosition, 1.0);
    vec4 viewPosition = uViewMatrix * worldPosition;
    // Linear depth is not flipped.
    LinearDepth = -viewPosition.z;
    
    //Near and far are flipped for better precision.
    gl_Position = uFlippedViewProjectionMatrix * worldPosition;
}