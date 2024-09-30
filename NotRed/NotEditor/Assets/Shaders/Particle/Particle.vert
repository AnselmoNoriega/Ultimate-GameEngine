#version 450 core

const vec3 VERTICES[6] = {
	vec3(-0.5, 0.0, -0.5),
	vec3( 0.5, 0.0, -0.5),
	vec3(-0.5, 0.0,  0.5),
	vec3( 0.5, 0.0, -0.5),
	vec3(-0.5, 0.0,  0.5), 
	vec3( 0.5, 0.0,  0.5)
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
	mat4 uInverseViewProjectionMatrix;
	mat4 uViewMatrix;
};

layout (std140, binding = 1) uniform ShadowData
{
	mat4 uLightMatrix[4];
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} uRenderer;

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	
	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	vec3 a_pos = VERTICES[gl_VertexIndex % 6];

	gl_Position = uViewProjectionMatrix  * vec4(a_pos, 1.0);
}