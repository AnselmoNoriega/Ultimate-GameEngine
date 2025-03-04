// -----------------------------
// -- Hazel Engine PBR shader --
// -----------------------------
// Note: this shader is still very much in progress. There are likely many bugs and future additions that will go in.
//       Currently heavily updated. 
//
// References upon which this is based:
// - Unreal Engine 4 PBR notes (https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013pbsepicnotesv2.pdf)
// - Frostbite's SIGGRAPH 2014 paper (https://seblagarde.wordpress.com/2015/07/14/siggraph-2014-moving-frostbite-to-physically-based-rendering/)
// - Micha Siejak's PBR project (https://github.com/Nadrin)
// - My implementation from years ago in the Sparky engine (https://github.com/TheCherno/Sparky)
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

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
	mat4 uInverseViewProjectionMatrix;
	mat4 uProjectionMatrix;
	mat4 uViewMatrix;
};

layout (std140, binding = 1) uniform ShadowData
{
	mat4 uLightMatrix[4];
};

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;

	mat3 CameraView;

	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
};

layout (location = 0) out VertexOutput Output;

void main()
{
	mat4 transform = mat4(
		vec4(aMRow0.x, aMRow1.x, aMRow2.x, 0.0),
		vec4(aMRow0.y, aMRow1.y, aMRow2.y, 0.0),
		vec4(aMRow0.z, aMRow1.z, aMRow2.z, 0.0),
		vec4(aMRow0.w, aMRow1.w, aMRow2.w, 1.0)
	);

	//Output.WorldPosition = vec3(u_Renderer.Transform * vec4(aPosition, 1.0));
	Output.WorldPosition = vec3(transform * vec4(aPosition, 1.0));
	Output.Normal = mat3(transform) * aNormal;
	Output.TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
	Output.WorldNormals = mat3(transform) * mat3(aTangent, aBinormal, aNormal);
	Output.WorldTransform = mat3(transform);
	Output.Binormal = aBinormal;

	Output.CameraView = mat3(uViewMatrix);

	Output.ShadowMapCoords[0] = uLightMatrix[0] * vec4(Output.WorldPosition, 1.0);
	Output.ShadowMapCoords[1] = uLightMatrix[1] * vec4(Output.WorldPosition, 1.0);
	Output.ShadowMapCoords[2] = uLightMatrix[2] * vec4(Output.WorldPosition, 1.0);
	Output.ShadowMapCoords[3] = uLightMatrix[3] * vec4(Output.WorldPosition, 1.0);
	Output.ViewPosition = vec3(uViewMatrix * vec4(Output.WorldPosition, 1.0));
	
	gl_Position = uViewProjectionMatrix * transform * vec4(aPosition, 1.0);
}