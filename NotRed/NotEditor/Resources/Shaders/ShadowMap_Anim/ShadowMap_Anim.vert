#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;
layout(location = 5) in ivec4 aBoneIndices;
layout(location = 6) in vec4 aBoneWeights;

layout (std140, binding = 1) uniform ShadowData
{
	mat4 uViewProjectionMatrix[4];
};

const int MAX_BONES = 100;

layout (std140, set = 1, binding = 0) uniform BoneTransforms
{
	mat4 BoneTransforms[MAX_BONES];
} uBoneTransforms;

layout (push_constant) uniform Transform
{
	mat4 Transform;
	uint Cascade;
} uRenderer;

void main()
{
	mat4 boneTransform = uBoneTransforms.BoneTransforms[aBoneIndices[0]] * aBoneWeights[0];
	boneTransform += uBoneTransforms.BoneTransforms[aBoneIndices[1]] * aBoneWeights[1];
	boneTransform += uBoneTransforms.BoneTransforms[aBoneIndices[2]] * aBoneWeights[2];
	boneTransform += uBoneTransforms.BoneTransforms[aBoneIndices[3]] * aBoneWeights[3];

	gl_Position = uViewProjectionMatrix[uRenderer.Cascade] * uRenderer.Transform * boneTransform * vec4(aPosition, 1.0);
}