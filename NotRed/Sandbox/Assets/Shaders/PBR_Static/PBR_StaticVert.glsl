#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

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
	vsOutput.WorldPosition = vec3(uModelMatrix * vec4(aPosition, 1.0));
    vsOutput.Normal = aNormal;
	vsOutput.TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
	vsOutput.WorldNormals = mat3(uModelMatrix) * mat3(aTangent, aBinormal, aNormal);
	vsOutput.Binormal = aBinormal;

	gl_Position = uViewProjectionMatrix * uModelMatrix * vec4(aPosition, 1.0);
}