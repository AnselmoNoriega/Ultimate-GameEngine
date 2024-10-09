// References upon which this is based:
// - Unreal Engine 4 PBR notes (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf)
// - Frostbite's SIGGRAPH 2014 paper (https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)
// - Michał Siejak's PBR project (https://github.com/Nadrin)
// - The Cherno from Sparky engine (https://github.com/TheCherno/Sparky)
// - The Cherno from Hazel engine ()
#type vertex
#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
	mat4 uInverseViewProjectionMatrix;
	mat4 uProjectionMatrix;
	mat4 uViewMatrix;
};

layout (std140, binding = 1) uniform ShadowData
{
	mat4 uLightMatrixCascade0;
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

	vec4 ShadowMapCoords;
	vec4 ShadowMapCoordsBiased;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	Output.WorldPosition = vec3(uRenderer.Transform * vec4(aPosition, 1.0));
    Output.Normal = mat3(uRenderer.Transform) * aNormal;
	Output.TexCoord = aTexCoord;
	Output.WorldNormals = mat3(uRenderer.Transform) * mat3(aTangent, aBinormal, aNormal);
	Output.WorldTransform = mat3(uRenderer.Transform);
	Output.Binormal = aBinormal;

	Output.ShadowMapCoords = uLightMatrixCascade0 * vec4(Output.WorldPosition, 1.0);
	Output.ShadowMapCoordsBiased = uLightMatrixCascade0 * vec4(Output.WorldPosition, 1.0);

	gl_Position = uViewProjectionMatrix * uRenderer.Transform * vec4(aPosition, 1.0);
}