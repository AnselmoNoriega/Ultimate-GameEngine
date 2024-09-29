#version 450 core

#define NUM_VERTICES 6
const vec3 VERTICES[NUM_VERTICES] = {
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
	//vec3 a_pos = VERTICES[gl_VertexIndex % NUM_VERTICES];
	//vec2 a_texPos = VERTICES[gl_VertexIndex % NUM_VERTICES].xz + vec2(0.5);
	
	gl_Position = uViewProjectionMatrix * uRenderer.Transform * vec4(aPosition, 1.0);
}