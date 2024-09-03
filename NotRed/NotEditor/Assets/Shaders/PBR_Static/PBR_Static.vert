#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

uniform mat4 uViewProjectionMatrix;
uniform mat4 uViewMatrix;
uniform mat4 uTransform;

uniform mat4 uLightMatrixCascade0;
uniform mat4 uLightMatrixCascade1;
uniform mat4 uLightMatrixCascade2;
uniform mat4 uLightMatrixCascade3;

out VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
} vsOutput;

void main()
{
	vsOutput.WorldPosition = vec3(uTransform * vec4(aPosition, 1.0));
    vsOutput.Normal = mat3(uTransform) * aNormal;
	vsOutput.TexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
	vsOutput.WorldNormals = mat3(uTransform) * mat3(aTangent, aBinormal, aNormal);
	vsOutput.WorldTransform = mat3(uTransform);
	vsOutput.Binormal = aBinormal;

	vsOutput.ShadowMapCoords[0] = uLightMatrixCascade0 * vec4(vsOutput.WorldPosition, 1.0);
	vsOutput.ShadowMapCoords[1] = uLightMatrixCascade1 * vec4(vsOutput.WorldPosition, 1.0);
	vsOutput.ShadowMapCoords[2] = uLightMatrixCascade2 * vec4(vsOutput.WorldPosition, 1.0);
	vsOutput.ShadowMapCoords[3] = uLightMatrixCascade3 * vec4(vsOutput.WorldPosition, 1.0);
	vsOutput.ViewPosition = vec3(uViewMatrix * vec4(vsOutput.WorldPosition, 1.0));

	gl_Position = uViewProjectionMatrix * uTransform * vec4(aPosition, 1.0);
}