#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

const int LightCount = 1;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

struct PointLight {
	vec3 Position;
	float Multiplier;
	vec3 Radiance;
	float MinRadius;
	float Radius;
	float Falloff;
	float LightSize;
	bool CastsShadows;
};

layout(std140, binding = 3) uniform RendererData
{
	uniform vec4 uCascadeSplits;
	uniform int uTilesCountX;
	uniform bool uShowCascades;
	uniform bool uSoftShadows;
	uniform float uLightSize;
	uniform float uMaxShadowDistance;
	uniform float uShadowFade;
	uniform bool uCascadeFading;
	uniform float uCascadeTransitionFade;
	uniform bool uShowLightComplexity;
};

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

	mat3 CameraView;

	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
};

layout(location = 0) in VertexOutput Input;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 oViewNormals;
layout(location = 2) out vec4 oViewPosition;

layout(std140, binding = 4) uniform SceneData
{
	DirectionalLight uDirectionalLights;
	vec3 uCameraPosition; // Offset = 32
	float uEnvironmentMapIntensity;
};

layout(std140, binding = 5) uniform PointLightData
{
	uint uPointLightsCount;
	PointLight upointLights[1024];
};

// PBR texture inputs
layout(set = 0, binding = 6) uniform sampler2D uAlbedoTexture;
layout(set = 0, binding = 7) uniform sampler2D uNormalTexture;
layout(set = 0, binding = 8) uniform sampler2D uMetalnessTexture;
layout(set = 0, binding = 9) uniform sampler2D uRoughnessTexture;

// Environment maps
layout(set = 1, binding = 10) uniform samplerCube uEnvRadianceTex;
layout(set = 1, binding = 11) uniform samplerCube uEnvIrradianceTex;

// BRDF LUT
layout(set = 1, binding = 12) uniform sampler2D uBRDFLUTTexture;

// Shadow maps
layout(set = 1, binding = 13) uniform sampler2DArray uShadowMapTexture;
layout(std430, binding = 14) readonly buffer VisibleLightIndicesBuffer {
	int indices[];
} visibleLightIndicesBuffer;

//HBAO Linear Depth
layout(set = 1, binding = 15) uniform sampler2D uLinearDepthTex;

layout(std140, binding = 16) uniform ScreenData
{
	vec2 uInvFullResolution;
	vec2 uFullResolution;
};

layout(std140, binding = 17) uniform HBAOData
{
	vec4	uPerspectiveInfo;   // R = (x) * (R - L)/N \\\\\\ G = (y) * (T - B)/N \\\\\\ B =  L/N \\\\\\ A =  B/N
	vec2    uInvQuarterResolution;
	float   uRadiusToScreen;        // radius
	float   uNegInvR2;     // radius * radius
	float   uNDotVBias;
	float   uAOMultiplier;
	float   uPowExponent;
	bool	uIsOrtho;
	vec4    uFloat2Offsets[16];
	vec4    uJitters[16];
};

layout(push_constant) uniform Material
{
	layout(offset = 64) vec3 AlbedoColor;
	float Metalness;
	float Roughness;
	float Emission;

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

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below.
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float gaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return gaSchlickG1(cosLi, k) * gaSchlickG1(NdotV, k);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

// Shlick's approximation of the Fresnel factor.
vec3 fresnelSchlick(vec3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// ---------------------------------------------------------------------------------------------------
// The following code (from Unreal Engine 4's paper) shows how to filter the environment map
// for different roughnesses. This is mean to be computed offline and stored in cube map mips,
// so turning this on online will cause poor performance
float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
	float a = Roughness * Roughness;
	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);
	vec3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;
	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
	vec3 TangentX = normalize(cross(UpVector, N));
	vec3 TangentY = cross(N, TangentX);
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
}

float TotalWeight = 0.0;

vec3 PrefilterEnvMap(float Roughness, vec3 R)
{
	vec3 N = R;
	vec3 V = R;
	vec3 PrefilteredColor = vec3(0.0);
	int NumSamples = 1024;
	for (int i = 0; i < NumSamples; i++)
	{
		vec2 Xi = Hammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
		vec3 L = 2 * dot(V, H) * H - V;
		float NoL = clamp(dot(N, L), 0.0, 1.0);
		if (NoL > 0)
		{
			//PrefilteredColor += texture(uEnvRadianceTex, L).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

// ---------------------------------------------------------------------------------------------------

vec3 RotateVectorAboutY(float angle, vec3 vec)
{
	angle = radians(angle);
	mat3x3 rotationMatrix = { vec3(cos(angle),0.0,sin(angle)),
							vec3(0.0,1.0,0.0),
							vec3(-sin(angle),0.0,cos(angle)) };
	return rotationMatrix * vec;
}

vec3 CalculateDirLights(vec3 F0)
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < LightCount; i++)
	{
		vec3 Li = uDirectionalLights.Direction;
		vec3 Lradiance = uDirectionalLights.Radiance * uDirectionalLights.Multiplier;
		vec3 Lh = normalize(Li + mParams.View);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(mParams.Normal, Li));
		float cosLh = max(0.0, dot(mParams.Normal, Lh));

		vec3 F = fresnelSchlickRoughness(F0, max(0.0, dot(Lh, mParams.View)), mParams.Roughness);
		float D = ndfGGX(cosLh, mParams.Roughness);
		float G = gaSchlickGGX(cosLi, mParams.NdotV, mParams.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - mParams.Metalness);
		vec3 diffuseBRDF = kd * mParams.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * mParams.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
}

int GetLightBufferIndex(int i)
{
	ivec2 tileID = ivec2(gl_FragCoord) / ivec2(16, 16);
	uint index = tileID.y * uTilesCountX + tileID.x;

	uint offset = index * 1024;
	return visibleLightIndicesBuffer.indices[offset + i];
}

int GetPointLightCount()
{
	int result = 0;
	for (int i = 0; i < uPointLightsCount; i++)
	{
		uint lightIndex = GetLightBufferIndex(i);
		if (lightIndex == -1)
			break;

		result++;
	}

	return result;
}

vec3 CalculatePointLights(in vec3 F0)
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < uPointLightsCount; i++)
	{
		uint lightIndex = GetLightBufferIndex(i);
		if (lightIndex == -1)
			break;

		PointLight light = upointLights[lightIndex];
		vec3 Li = normalize(light.Position - Input.WorldPosition);
		float lightDistance = length(light.Position - Input.WorldPosition);
		vec3 Lh = normalize(Li + mParams.View);

		float attenuation = clamp(1.0 - (lightDistance * lightDistance) / (light.Radius * light.Radius), 0.0, 1.0);
		attenuation *= mix(attenuation, 1.0, light.Falloff);

		vec3 Lradiance = light.Radiance * light.Multiplier * attenuation;

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(mParams.Normal, Li));
		float cosLh = max(0.0, dot(mParams.Normal, Lh));

		vec3 F = fresnelSchlickRoughness(F0, max(0.0, dot(Lh, mParams.View)), mParams.Roughness);
		float D = ndfGGX(cosLh, mParams.Roughness);
		float G = gaSchlickGGX(cosLi, mParams.NdotV, mParams.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - mParams.Metalness);
		vec3 diffuseBRDF = kd * mParams.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * mParams.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
}

float Convert_sRGB_FromLinear(float theLinearValue)
{
	return theLinearValue <= 0.0031308f
		? theLinearValue * 12.92f
		: pow(theLinearValue, 1.0f / 2.4f) * 1.055f - 0.055f;
}

vec3 IBL(vec3 F0, vec3 Lr)
{
	vec3 irradiance = texture(uEnvIrradianceTex, mParams.Normal).rgb;
	vec3 F = fresnelSchlickRoughness(F0, mParams.NdotV, mParams.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - mParams.Metalness);
	vec3 diffuseIBL = mParams.Albedo * irradiance;

	int envRadianceTexLevels = textureQueryLevels(uEnvRadianceTex);
	float NoV = clamp(mParams.NdotV, 0.0, 1.0);
	vec3 R = 2.0 * dot(mParams.View, mParams.Normal) * mParams.Normal - mParams.View;
	vec3 specularIrradiance = textureLod(uEnvRadianceTex, RotateVectorAboutY(uMaterialUniforms.EnvMapRotation, Lr), (mParams.Roughness) * envRadianceTexLevels).rgb;
	//specularIrradiance = vec3(Convert_sRGB_FromLinear(specularIrradiance.r), Convert_sRGB_FromLinear(specularIrradiance.g), Convert_sRGB_FromLinear(specularIrradiance.b));

	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
	vec2 specularBRDF = texture(uBRDFLUTTexture, vec2(mParams.NdotV, 1.0 - mParams.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

/////////////////////////////////////////////
// PCSS
/////////////////////////////////////////////

float ShadowFade = 1.0;

float GetShadowBias()
{
	const float MINIMUmSHADOW_BIAS = 0.002;
	float bias = max(MINIMUmSHADOW_BIAS * (1.0 - dot(mParams.Normal, uDirectionalLights.Direction)), MINIMUmSHADOW_BIAS);
	return bias;
}

float HardShadows_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords)
{
	float bias = GetShadowBias();
	float shadowMapDepth = texture(shadowMap, vec3(shadowCoords.xy * 0.5 + 0.5, cascade)).x;
	return step(shadowCoords.z, shadowMapDepth + bias) * ShadowFade;
}

// Penumbra

// this search area estimation comes from the following article: 
// http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float SearchWidth(float uvLightSize, float receiverDistance)
{
	const float NEAR = 0.1;
	return uvLightSize * (receiverDistance - NEAR) / uCameraPosition.z;
}

float SearchRegionRadiusUV(float zWorld)
{
	const float light_zNear = 0.0; // 0.01 gives artifacts? maybe because of ortho proj?
	const float lightRadiusUV = 0.05;
	return lightRadiusUV * (zWorld - light_zNear) / zWorld;
}

const vec2 PoissonDistribution[64] = vec2[](
	vec2(-0.884081, 0.124488),
	vec2(-0.714377, 0.027940),
	vec2(-0.747945, 0.227922),
	vec2(-0.939609, 0.243634),
	vec2(-0.985465, 0.045534),
	vec2(-0.861367, -0.136222),
	vec2(-0.881934, 0.396908),
	vec2(-0.466938, 0.014526),
	vec2(-0.558207, 0.212662),
	vec2(-0.578447, -0.095822),
	vec2(-0.740266, -0.095631),
	vec2(-0.751681, 0.472604),
	vec2(-0.553147, -0.243177),
	vec2(-0.674762, -0.330730),
	vec2(-0.402765, -0.122087),
	vec2(-0.319776, -0.312166),
	vec2(-0.413923, -0.439757),
	vec2(-0.979153, -0.201245),
	vec2(-0.865579, -0.288695),
	vec2(-0.243704, -0.186378),
	vec2(-0.294920, -0.055748),
	vec2(-0.604452, -0.544251),
	vec2(-0.418056, -0.587679),
	vec2(-0.549156, -0.415877),
	vec2(-0.238080, -0.611761),
	vec2(-0.267004, -0.459702),
	vec2(-0.100006, -0.229116),
	vec2(-0.101928, -0.380382),
	vec2(-0.681467, -0.700773),
	vec2(-0.763488, -0.543386),
	vec2(-0.549030, -0.750749),
	vec2(-0.809045, -0.408738),
	vec2(-0.388134, -0.773448),
	vec2(-0.429392, -0.894892),
	vec2(-0.131597, 0.065058),
	vec2(-0.275002, 0.102922),
	vec2(-0.106117, -0.068327),
	vec2(-0.294586, -0.891515),
	vec2(-0.629418, 0.379387),
	vec2(-0.407257, 0.339748),
	vec2(0.071650, -0.384284),
	vec2(0.022018, -0.263793),
	vec2(0.003879, -0.136073),
	vec2(-0.137533, -0.767844),
	vec2(-0.050874, -0.906068),
	vec2(0.114133, -0.070053),
	vec2(0.163314, -0.217231),
	vec2(-0.100262, -0.587992),
	vec2(-0.004942, 0.125368),
	vec2(0.035302, -0.619310),
	vec2(0.195646, -0.459022),
	vec2(0.303969, -0.346362),
	vec2(-0.678118, 0.685099),
	vec2(-0.628418, 0.507978),
	vec2(-0.508473, 0.458753),
	vec2(0.032134, -0.782030),
	vec2(0.122595, 0.280353),
	vec2(-0.043643, 0.312119),
	vec2(0.132993, 0.085170),
	vec2(-0.192106, 0.285848),
	vec2(0.183621, -0.713242),
	vec2(0.265220, -0.596716),
	vec2(-0.009628, -0.483058),
	vec2(-0.018516, 0.435703)
	);

const vec2 poissonDisk[16] = vec2[](
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760),
	vec2(-0.91588581, 0.45771432),
	vec2(-0.81544232, -0.87912464),
	vec2(-0.38277543, 0.27676845),
	vec2(0.97484398, 0.75648379),
	vec2(0.44323325, -0.97511554),
	vec2(0.53742981, -0.47373420),
	vec2(-0.26496911, -0.41893023),
	vec2(0.79197514, 0.19090188),
	vec2(-0.24188840, 0.99706507),
	vec2(-0.81409955, 0.91437590),
	vec2(0.19984126, 0.78641367),
	vec2(0.14383161, -0.14100790)
	);

vec2 SamplePoisson(int index)
{
	return PoissonDistribution[index % 64];
}

float FindBlockerDistance_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvLightSize)
{
	float bias = GetShadowBias();

	int numBlockerSearchSamples = 64;
	int blockers = 0;
	float avgBlockerDistance = 0;

	float searchWidth = SearchRegionRadiusUV(shadowCoords.z);
	for (int i = 0; i < numBlockerSearchSamples; i++)
	{
		float z = textureLod(shadowMap, vec3((shadowCoords.xy * 0.5 + 0.5) + SamplePoisson(i) * searchWidth, cascade), 0).r;
		if (z < (shadowCoords.z - bias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0)
		return avgBlockerDistance / float(blockers);

	return -1;
}

float PCF_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();
	int numPCFSamples = 64;

	float sum = 0;
	for (int i = 0; i < numPCFSamples; i++)
	{
		vec2 offset = SamplePoisson(i) * uvRadius;
		float z = textureLod(shadowMap, vec3((shadowCoords.xy * 0.5 + 0.5) + offset, cascade), 0).r;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / numPCFSamples;
}

float NV_PCF_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();

	float sum = 0;
	for (int i = 0; i < 16; i++)
	{
		vec2 offset = poissonDisk[i] * uvRadius;
		float z = textureLod(shadowMap, vec3((shadowCoords.xy * 0.5 + 0.5) + offset, cascade), 0).r;
		sum += step(shadowCoords.z - bias, z);
	}
	return sum / 16.0f;
}

float PCSS_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords, float uvLightSize)
{
	float blockerDistance = FindBlockerDistance_DirectionalLight(shadowMap, cascade, shadowCoords, uvLightSize);
	if (blockerDistance == -1) // No occlusion
		return 1.0f;

	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	float NEAR = 0.01; // Should this value be tweakable?
	float uvRadius = penumbraWidth * uvLightSize * NEAR / shadowCoords.z; // Do we need to divide by shadowCoords.z?
	uvRadius = min(uvRadius, 0.002f);
	return PCF_DirectionalLight(shadowMap, cascade, shadowCoords, uvRadius) * ShadowFade;
}

vec3 UVToView(vec2 uv, float eye_z)
{
	return vec3((uv * uPerspectiveInfo.xy + uPerspectiveInfo.zw) * (uIsOrtho ? 1.0 : eye_z), eye_z);
}

vec3 FetchViewPos(vec2 UV)
{
	float ViewDepth = textureLod(uLinearDepthTex, UV, 0).r;
	return UVToView(UV, ViewDepth);
}

vec3 MinDiff(vec3 P, vec3 Pr, vec3 Pl)
{
	vec3 V1 = Pr - P;
	vec3 V2 = P - Pl;
	return (dot(V1, V1) < dot(V2, V2)) ? V1 : V2;
}

vec3 ReconstructNormal(vec2 UV, vec3 P)
{
	vec3 pRight = FetchViewPos(UV + vec2(uInvFullResolution.x, 0));
	vec3 pLeft = FetchViewPos(UV + vec2(-uInvFullResolution.x, 0));
	vec3 pTop = FetchViewPos(UV + vec2(0, uInvFullResolution.y));
	vec3 pBottom = FetchViewPos(UV + vec2(0, -uInvFullResolution.y));
	return normalize(cross(MinDiff(P, pRight, pLeft), MinDiff(P, pTop, pBottom)));
}

/////////////////////////////////////////////

vec3 GetGradient(float value)
{
	vec3 zero = vec3(0.0, 0.0, 0.0);
	vec3 white = vec3(0.0, 0.1, 0.9);
	vec3 red = vec3(0.2, 0.9, 0.4);
	vec3 blue = vec3(0.8, 0.8, 0.3);
	vec3 green = vec3(0.9, 0.2, 0.3);

	float step0 = 0.0f;
	float step1 = 2.0f;
	float step2 = 4.0f;
	float step3 = 8.0f;
	float step4 = 16.0f;

	vec3 color = mix(zero, white, smoothstep(step0, step1, value));
	color = mix(color, white, smoothstep(step1, step2, value));
	color = mix(color, red, smoothstep(step1, step2, value));
	color = mix(color, blue, smoothstep(step2, step3, value));
	color = mix(color, green, smoothstep(step3, step4, value));

	return color;
}

void main()
{
	// Standard PBR inputs
	mParams.Albedo = texture(uAlbedoTexture, Input.TexCoord).rgb * uMaterialUniforms.AlbedoColor;
	mParams.Metalness = texture(uMetalnessTexture, Input.TexCoord).r * uMaterialUniforms.Metalness;
	mParams.Roughness = texture(uRoughnessTexture, Input.TexCoord).r * uMaterialUniforms.Roughness;
	mParams.Roughness = max(mParams.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight

	// Normals (either from vertex or map)
	mParams.Normal = normalize(Input.Normal);
	if (uMaterialUniforms.UseNormalMap)
	{
		mParams.Normal = normalize(texture(uNormalTexture, Input.TexCoord).rgb * 2.0f - 1.0f);
		mParams.Normal = normalize(Input.WorldNormals * mParams.Normal);
	}

	mParams.View = normalize(uCameraPosition - Input.WorldPosition);
	mParams.NdotV = max(dot(mParams.Normal, mParams.View), 0.0);


	// Specular reflection vector
	vec3 Lr = 2.0 * mParams.NdotV * mParams.Normal - mParams.View;

	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, mParams.Albedo, mParams.Metalness);

	uint cascadeIndex = 0;

	const uint SHADOW_MAP_CASCADE_COUNT = 4;
	for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; i++)
	{
		if (Input.ViewPosition.z < uCascadeSplits[i])
			cascadeIndex = i + 1;
	}

	float shadowDistance = uMaxShadowDistance;//uCascadeSplits[3];
	float transitionDistance = uShadowFade;
	float distance = length(Input.ViewPosition);
	ShadowFade = distance - (shadowDistance - transitionDistance);
	ShadowFade /= transitionDistance;
	ShadowFade = clamp(1.0 - ShadowFade, 0.0, 1.0);
	float shadowAmount = 1.0;

	bool fadeCascades = uCascadeFading;
	if (fadeCascades)
	{
		float cascadeTransitionFade = uCascadeTransitionFade;

		float c0 = smoothstep(uCascadeSplits[0] + cascadeTransitionFade * 0.5f, uCascadeSplits[0] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		float c1 = smoothstep(uCascadeSplits[1] + cascadeTransitionFade * 0.5f, uCascadeSplits[1] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		float c2 = smoothstep(uCascadeSplits[2] + cascadeTransitionFade * 0.5f, uCascadeSplits[2] - cascadeTransitionFade * 0.5f, Input.ViewPosition.z);
		if (c0 > 0.0 && c0 < 1.0)
		{
			// Sample 0 & 1
			vec3 shadowMapCoords = (Input.ShadowMapCoords[0].xyz / Input.ShadowMapCoords[0].w);
			float shadowAmount0 = uSoftShadows ? PCSS_DirectionalLight(uShadowMapTexture, 0, shadowMapCoords, uLightSize) : HardShadows_DirectionalLight(uShadowMapTexture, 0, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[1].xyz / Input.ShadowMapCoords[1].w);
			float shadowAmount1 = uSoftShadows ? PCSS_DirectionalLight(uShadowMapTexture, 1, shadowMapCoords, uLightSize) : HardShadows_DirectionalLight(uShadowMapTexture, 1, shadowMapCoords);

			shadowAmount = mix(shadowAmount0, shadowAmount1, c0);
		}
		else if (c1 > 0.0 && c1 < 1.0)
		{
			// Sample 1 & 2
			vec3 shadowMapCoords = (Input.ShadowMapCoords[1].xyz / Input.ShadowMapCoords[1].w);
			float shadowAmount1 = uSoftShadows ? PCSS_DirectionalLight(uShadowMapTexture, 1, shadowMapCoords, uLightSize) : HardShadows_DirectionalLight(uShadowMapTexture, 1, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[2].xyz / Input.ShadowMapCoords[2].w);
			float shadowAmount2 = uSoftShadows ? PCSS_DirectionalLight(uShadowMapTexture, 2, shadowMapCoords, uLightSize) : HardShadows_DirectionalLight(uShadowMapTexture, 2, shadowMapCoords);

			shadowAmount = mix(shadowAmount1, shadowAmount2, c1);
		}
		else if (c2 > 0.0 && c2 < 1.0)
		{
			// Sample 2 & 3
			vec3 shadowMapCoords = (Input.ShadowMapCoords[2].xyz / Input.ShadowMapCoords[2].w);
			float shadowAmount2 = uSoftShadows ? PCSS_DirectionalLight(uShadowMapTexture, 2, shadowMapCoords, uLightSize) : HardShadows_DirectionalLight(uShadowMapTexture, 2, shadowMapCoords);
			shadowMapCoords = (Input.ShadowMapCoords[3].xyz / Input.ShadowMapCoords[3].w);
			float shadowAmount3 = uSoftShadows ? PCSS_DirectionalLight(uShadowMapTexture, 3, shadowMapCoords, uLightSize) : HardShadows_DirectionalLight(uShadowMapTexture, 3, shadowMapCoords);

			shadowAmount = mix(shadowAmount2, shadowAmount3, c2);
		}
		else
		{
			vec3 shadowMapCoords = (Input.ShadowMapCoords[cascadeIndex].xyz / Input.ShadowMapCoords[cascadeIndex].w);
			shadowAmount = uSoftShadows ? PCSS_DirectionalLight(uShadowMapTexture, cascadeIndex, shadowMapCoords, uLightSize) : HardShadows_DirectionalLight(uShadowMapTexture, cascadeIndex, shadowMapCoords);
		}
	}
	else
	{
		vec3 shadowMapCoords = (Input.ShadowMapCoords[cascadeIndex].xyz / Input.ShadowMapCoords[cascadeIndex].w);
		shadowAmount = uSoftShadows ? PCSS_DirectionalLight(uShadowMapTexture, cascadeIndex, shadowMapCoords, uLightSize) : HardShadows_DirectionalLight(uShadowMapTexture, cascadeIndex, shadowMapCoords);
	}


	vec3 lightContribution = CalculateDirLights(F0) * shadowAmount;
	lightContribution += CalculatePointLights(F0);
	lightContribution += mParams.Albedo * uMaterialUniforms.Emission;
	vec3 iblContribution = IBL(F0, Lr) * uEnvironmentMapIntensity;

	color = vec4(iblContribution + lightContribution, 1.0);
	
	// View normals for HBAO
	vec3 hbaoNormal = Input.CameraView * normalize(Input.Normal);
	hbaoNormal = hbaoNormal * 0.5 + 0.5;
	hbaoNormal.x = 1.0f - hbaoNormal.x;
	hbaoNormal.y = 1.0f - hbaoNormal.y;
	oViewNormals = vec4(hbaoNormal, 1.0f);

	oViewPosition = vec4(Input.ViewPosition, 1.0);
	
	if (uShowLightComplexity)
	{
		int pointLightCount = GetPointLightCount();
		float value = float(pointLightCount);
		color.rgb = (color.rgb * 0.2) + GetGradient(value);
	}

	// (shading-only)
	// color.rgb = vec3(1.0) * shadowAmount + 0.2f;

	if (uShowCascades)
	{
		switch (cascadeIndex)
		{
		case 0:
			color.rgb *= vec3(1.0f, 0.25f, 0.25f);
			break;
		case 1:
			color.rgb *= vec3(0.25f, 1.0f, 0.25f);
			break;
		case 2:
			color.rgb *= vec3(0.25f, 0.25f, 1.0f);
			break;
		case 3:
			color.rgb *= vec3(1.0f, 1.0f, 0.25f);
			break;
		}
	}
}