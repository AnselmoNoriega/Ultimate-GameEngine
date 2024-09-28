#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

const int LightCount = 1;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

struct DirectionalLight
{
	vec3 Direction;
	vec3 Radiance;
	float Multiplier;
};

struct VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;	
	
	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
};

layout (location = 0) in VertexOutput Input;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 oBloomColor;

layout (std140, binding = 2) uniform SceneData
{
	DirectionalLight uDirectionalLights;
	vec3 uCameraPosition; // Offset = 32
	bool uHasEnvironmentMap;
};

layout (std140, binding = 3) uniform RendererData
{
	vec4 uCascadeSplits;
	bool uShowCascades;
	bool uSoftShadows;
	float uLightSize;
	float uMaxShadowDistance;
	float uShadowFade;
	bool uCascadeFading;
	float uCascadeTransitionFade;
};

// PBR texture inputs
layout (set = 0, binding = 4) uniform sampler2D uAlbedoTexture;
layout (set = 0, binding = 5) uniform sampler2D uNormalTexture;
layout (set = 0, binding = 6) uniform sampler2D uMetalnessTexture;
layout (set = 0, binding = 7) uniform sampler2D uRoughnessTexture;

// Environment maps
layout (set = 1, binding = 8) uniform samplerCube uEnvRadianceTex;
layout (set = 1, binding = 9) uniform samplerCube uEnvIrradianceTex;

// BRDF LUT
layout (set = 1, binding = 10) uniform sampler2D uBRDFLUTTexture;

// Shadow maps
layout (set = 1, binding = 11) uniform sampler2DArray uShadowMapTexture;

layout (push_constant) uniform Material
{
	layout (offset = 64) vec3 AlbedoColor;
	float Metalness;
	float Roughness;

	float EnvMapRotation;

	bool UseNormalMap;
} uMaterialUniforms;

struct PBRParameters
{
	vec3 Albedo;
	float Roughness;
	float Metalness;

	vec3 Normal;
	vec3 View;
	float NdotV;
};

PBRParameters mParams;

/////////////////////////////////////////////

void main() 
{
	oBloomColor = vec4(1.0, 0.0, 1.0, 1.0);
    color = vec4(1.0, 0.0, 0.0, 1.0);  // Red color
}