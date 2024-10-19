#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

layout(location = 5) in ivec4 aBoneIndices;
layout(location = 6) in vec4 aBoneWeights;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
};

uniform mat4 uLightMatrixCascade0;
uniform mat4 uLightMatrixCascade1;
uniform mat4 uLightMatrixCascade2;
uniform mat4 uLightMatrixCascade3;

const int MAX_BONES = 100;
layout (std140, binding = 1) uniform Transform
{
	mat4 uTransform;
	mat4 uBoneTransforms[100];
};

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	mat4 boneTransform = uBoneTransforms[aBoneIndices[0]] * aBoneWeights[0];
    boneTransform += uBoneTransforms[aBoneIndices[1]] * aBoneWeights[1];
    boneTransform += uBoneTransforms[aBoneIndices[2]] * aBoneWeights[2];
    boneTransform += uBoneTransforms[aBoneIndices[3]] * aBoneWeights[3];

	vec4 localPosition = boneTransform * vec4(aPosition, 1.0);

	Output.WorldPosition = vec3(uTransform * boneTransform * vec4(aPosition, 1.0));
    Output.Normal = mat3(uTransform) * mat3(boneTransform) * aNormal;
	Output.TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
	Output.WorldNormals = mat3(uTransform) * mat3(aTangent, aBinormal, aNormal);
	Output.Binormal = mat3(boneTransform) * aBinormal;

	gl_Position = uViewProjectionMatrix * uTransform * localPosition;
}