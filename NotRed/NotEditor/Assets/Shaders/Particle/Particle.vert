#version 450 core

layout(location = 0) in vec3 aPosition;

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
	gl_Position = uViewProjectionMatrix  * uRenderer.Transform * vec4(aPosition, 1.0);
	//gl_Position = uViewProjectionMatrix  * vec4(aPosition, 1.0);
}