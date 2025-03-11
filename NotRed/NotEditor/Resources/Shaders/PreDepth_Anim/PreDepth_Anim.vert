#version 450 core

// Vertex buffer
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

// Transform buffer
layout(location = 5) in vec4 aMRow0;
layout(location = 6) in vec4 aMRow1;
layout(location = 7) in vec4 aMRow2;

// Bone influences
layout(location = 8) in ivec4 aBoneIndices;
layout(location = 9) in vec4 aBoneWeights;

layout(std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
	mat4 uInverseViewProjectionMatrix;
	mat4 uProjectionMatrix;
	mat4 uViewMatrix;
};

const int MAXBONES = 100;
const int MAX_ANIMATED_MESHES = 1024;

layout (std140, set = 1, binding = 0) readonly buffer BoneTransforms
{
	mat4 BoneTransforms[MAX_BONES * MAX_ANIMATED_MESHES];
} rBoneTransforms;

layout(push_constant) uniform BoneTransformIndex
{
	uint Base;
} uBoneTransformIndex;

layout(location = 0) out float LinearDepth;

void main()
{
	mat4 transform = mat4(
		vec4(aMRow0.x, aMRow1.x, aMRow2.x, 0.0),
		vec4(aMRow0.y, aMRow1.y, aMRow2.y, 0.0),
		vec4(aMRow0.z, aMRow1.z, aMRow2.z, 0.0),
		vec4(aMRow0.w, aMRow1.w, aMRow2.w, 1.0)
	);
	
	mat4 boneTransform = rBoneTransforms.BoneTransforms[(uBoneTransformIndex.Base + gl_InstanceIndex) * MAX_BONES + aBoneIndices[0]] * aBoneWeights[0];
	boneTransform     += rBoneTransforms.BoneTransforms[(uBoneTransformIndex.Base + gl_InstanceIndex) * MAX_BONES + aBoneIndices[1]] * aBoneWeights[1];
	boneTransform     += rBoneTransforms.BoneTransforms[(uBoneTransformIndex.Base + gl_InstanceIndex) * MAX_BONES + aBoneIndices[2]] * aBoneWeights[2];
	boneTransform     += rBoneTransforms.BoneTransforms[(uBoneTransformIndex.Base + gl_InstanceIndex) * MAX_BONES + aBoneIndices[3]] * aBoneWeights[3];

	vec4 worldPosition = transform * boneTransform * vec4(aPosition, 1.0);

	LinearDepth = -(uViewMatrix * worldPosition).z;

	gl_Position = uViewProjectionMatrix * worldPosition;
}