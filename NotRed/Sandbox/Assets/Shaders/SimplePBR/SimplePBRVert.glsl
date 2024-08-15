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
} vs_Output;

void main()
{
	vs_Output.WorldPosition = vec3(mat4(uModelMatrix) * vec4(aPosition, 1.0));
    vs_Output.Normal = aNormal;
	vs_Output.TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
	vs_Output.WorldNormals = mat3(uModelMatrix) * mat3(aTangent, aBinormal, aNormal);

	gl_Position = uViewProjectionMatrix * uModelMatrix * vec4(aPosition, 1.0);
}