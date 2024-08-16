#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

layout(location = 5) in ivec4 aBoneIndices;
layout(location = 6) in vec4 aBoneWeights;

const int MAX_BONES = 100;
uniform mat4 uBoneTransforms[100];

uniform mat4 uViewProjectionMatrix;
uniform mat4 uModelMatrix;

out VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	vec3 Binormal;
} vsOutput;

void main()
{
	mat4 boneTransform = uBoneTransforms[aBoneIndices[0]] * aBoneWeights[0];
    boneTransform += uBoneTransforms[aBoneIndices[1]] * aBoneWeights[1];
    boneTransform += uBoneTransforms[aBoneIndices[2]] * aBoneWeights[2];
    boneTransform += uBoneTransforms[aBoneIndices[3]] * aBoneWeights[3];

	vec4 localPosition = boneTransform * vec4(aPosition, 1.0);

	vsOutput.WorldPosition = vec3(uModelMatrix * boneTransform * vec4(aPosition, 1.0));
    vsOutput.Normal = mat3(boneTransform) * aNormal;
	vsOutput.TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
	vsOutput.WorldNormals = mat3(uModelMatrix) * mat3(aTangent, aBinormal, aNormal);
	vsOutput.Binormal = mat3(boneTransform) * aBinormal;

	gl_Position = uViewProjectionMatrix * uModelMatrix * localPosition;
}