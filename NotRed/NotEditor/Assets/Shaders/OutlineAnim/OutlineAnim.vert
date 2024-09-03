#version 450 core

layout(location = 0) in vec3 aPosition;

layout(location = 5) in ivec4 aBoneIndices;
layout(location = 6) in vec4 aBoneWeights;

uniform mat4 uViewProjection;
uniform mat4 uTransform;

const int MAXBONES = 100;
uniform mat4 uBoneTransforms[100];

void main()
{
	mat4 boneTransform = uBoneTransforms[aBoneIndices[0]] * aBoneWeights[0];
    boneTransform += uBoneTransforms[aBoneIndices[1]] * aBoneWeights[1];
    boneTransform += uBoneTransforms[aBoneIndices[2]] * aBoneWeights[2];
    boneTransform += uBoneTransforms[aBoneIndices[3]] * aBoneWeights[3];

	vec4 localPosition = boneTransform * vec4(aPosition, 1.0);
	gl_Position = uViewProjection * uTransform * localPosition;
}