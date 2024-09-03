
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

in VertexOutput
{
	vec3 WorldPosition;
    vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	vec4 ShadowMapCoords[4];
	vec3 ViewPosition;
} vsInput;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 oBloomColor;

uniform DirectionalLight uDirectionalLights;
uniform vec3 uCameraPosition;

// PBR texture inputs
uniform sampler2D uAlbedoTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uMetalnessTexture;
uniform sampler2D uRoughnessTexture;

// Environment maps
uniform samplerCube uEnvRadianceTex;
uniform samplerCube uEnvIrradianceTex;

// BRDF LUT
uniform sampler2D uBRDFLUTTexture;

// PCSS
uniform sampler2D uShadowMapTexture[4];
uniform mat4 uLightView;
uniform bool uShowCascades;
uniform bool uSoftShadows;
uniform float uLightSize;
uniform float uMaxShadowDistance;
uniform float uShadowFade;
uniform bool uCascadeFading;
uniform float uCascadeTransitionFade;

uniform vec4 uCascadeSplits;

uniform float uIBLContribution;

uniform float uBloomThreshold;

////////////////////////////////////////

uniform vec3 uAlbedoColor;
uniform float uMetalness;
uniform float uRoughness;

uniform float uEnvMapRotation;

// Toggles
uniform float uAlbedoTexToggle;
uniform float uNormalTexToggle;
uniform float uMetalnessTexToggle;
uniform float uRoughnessTexToggle;

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
    float k = (r*r) / 8.0;

    float nom   = NdotV;
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
float RadicalInverseVdC(uint bits) 
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
    return vec2(float(i)/float(N), RadicalInverseVdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
	float a = Roughness * Roughness;
	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
	float SinTheta = sqrt( 1 - CosTheta * CosTheta );
	vec3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;
	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
	vec3 TangentX = normalize( cross( UpVector, N ) );
	vec3 TangentY = cross( N, TangentX );
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
	for(int i = 0; i < NumSamples; ++i)
	{
		vec2 Xi = Hammersley(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
		vec3 L = 2 * dot(V, H) * H - V;
		float NoL = clamp(dot(N, L), 0.0, 1.0);
		if (NoL > 0)
		{
			PrefilteredColor += texture(uEnvRadianceTex, L).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

// ---------------------------------------------------------------------------------------------------

vec3 RotateVectorAboutY(float angle, vec3 vec)
{
    angle = radians(angle);
    mat3x3 rotationMatrix ={vec3(cos(angle),0.0,sin(angle)),
                            vec3(0.0,1.0,0.0),
                            vec3(-sin(angle),0.0,cos(angle))};
    return rotationMatrix * vec;
}

vec3 Lighting(vec3 F0)
{
	vec3 result = vec3(0.0);
	for(int i = 0; i < LightCount; ++i)
	{
		vec3 Li = uDirectionalLights.Direction;
		vec3 Lradiance = uDirectionalLights.Radiance * uDirectionalLights.Multiplier;
		vec3 Lh = normalize(Li + mParams.View);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(mParams.Normal, Li));
		float cosLh = max(0.0, dot(mParams.Normal, Lh));

		vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, mParams.View)));
		float D = ndfGGX(cosLh, mParams.Roughness);
		float G = gaSchlickGGX(cosLi, mParams.NdotV, mParams.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - mParams.Metalness);
		vec3 diffuseBRDF = kd * mParams.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * mParams.NdotV);

		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
}

vec3 IBL(vec3 F0, vec3 Lr)
{
	vec3 irradiance = texture(uEnvIrradianceTex, mParams.Normal).rgb;
	vec3 F = fresnelSchlickRoughness(F0, mParams.NdotV, mParams.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - mParams.Metalness);
	vec3 diffuseIBL = mParams.Albedo * irradiance;

	int uEnvRadianceTexLevels = textureQueryLevels(uEnvRadianceTex);
	float NoV = clamp(mParams.NdotV, 0.0, 1.0);
	vec3 R = 2.0 * dot(mParams.View, mParams.Normal) * mParams.Normal - mParams.View;
	vec3 specularIrradiance = textureLod(uEnvRadianceTex, RotateVectorAboutY(uEnvMapRotation, Lr), (mParams.Roughness) * uEnvRadianceTexLevels).rgb;

	// Sample BRDF Lut, 1.0 - roughness for y-coord because texture was generated (in Sparky) for gloss model
	vec2 specularBRDF = texture(uBRDFLUTTexture, vec2(mParams.NdotV, 1.0 - mParams.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}

/////////////////////////////////////////////
// PCSS
/////////////////////////////////////////////

uint CascadeIndex = 0;
float ShadowFade = 1.0;

float GetShadowBias()
{
	const float MINIMUMSHADOWBIAS = 0.002;
	float bias = max(MINIMUMSHADOWBIAS * (1.0 - dot(mParams.Normal, uDirectionalLights.Direction)), MINIMUMSHADOWBIAS);
	return bias;
}

float HardShadowsDirectionalLight(sampler2D shadowMap, vec3 shadowCoords)
{
	float bias = GetShadowBias();
	float z = texture(shadowMap, shadowCoords.xy).x;
	return 1.0 - step(z + bias, shadowCoords.z) * ShadowFade;
}

// Penumbra

// this search area estimation comes from the following article: 
// http://developer.download.nvidia.com/whitepapers/2008/PCSSDirectionalLightIntegration.pdf
float SearchWidth(float uvLightSize, float receiverDistance)
{
	const float NEAR = 0.1;
	return uvLightSize * (receiverDistance - NEAR) / uCameraPosition.z;
}

float ulightzNear = 0.0; // 0.01 gives artifacts? maybe because of ortho proj?
float ulightzFar = 10000.0;
vec2 ulightRadiusUV = vec2(0.05);

vec2 searchRegionRadiusUV(float zWorld)
{
    return ulightRadiusUV * (zWorld - ulightzNear) / zWorld;
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

vec2 SamplePoisson(int index)
{
   return PoissonDistribution[index % 64];
}

float FindBlockerDistanceDirectionalLight(sampler2D shadowMap, vec3 shadowCoords, float uvLightSize)
{
	float bias = GetShadowBias();

	int numBlockerSearchSamples = 64;
	int blockers = 0;
	float avgBlockerDistance = 0;
	
	float zEye = -(uLightView * vec4(vsInput.WorldPosition, 1.0)).z;
	vec2 searchWidth = searchRegionRadiusUV(zEye);
	for (int i = 0; i < numBlockerSearchSamples; ++i)
	{
		float z = texture(shadowMap, shadowCoords.xy + SamplePoisson(i) * searchWidth).r;
		if (z < (shadowCoords.z - bias))
		{
			blockers++;
			avgBlockerDistance += z;
		}
	}

	if (blockers > 0)
	{
		return avgBlockerDistance / float(blockers);
	}

	return -1;
}

float PenumbraWidth(sampler2D shadowMap, vec3 shadowCoords, float uvLightSize)
{
	float blockerDistance = FindBlockerDistanceDirectionalLight(shadowMap, shadowCoords, uvLightSize);
	if (blockerDistance == -1)
		return -1;	
	
	return (shadowCoords.z - blockerDistance) / blockerDistance;
}

float PCFDirectionalLight(sampler2D shadowMap, vec3 shadowCoords, float uvRadius)
{
	float bias = GetShadowBias();
	int numPCFSamples = 64;
	float sum = 0;
	for (int i = 0; i < numPCFSamples; ++i)
	{
		float z = texture(shadowMap, shadowCoords.xy + SamplePoisson(i)  * uvRadius).r;
		sum += (z < (shadowCoords.z - bias)) ? 1 : 0;
	}
	return sum / numPCFSamples;
}

float PCSSDirectionalLight(sampler2D shadowMap, vec3 shadowCoords, float uvLightSize)
{
	float blockerDistance = FindBlockerDistanceDirectionalLight(shadowMap, shadowCoords, uvLightSize);
	if (blockerDistance == -1)
	{
		return 1;		
	}

	float penumbraWidth = (shadowCoords.z - blockerDistance) / blockerDistance;

	float NEAR = 0.01; // Should this value be tweakable?
	float uvRadius = penumbraWidth * uvLightSize * NEAR / shadowCoords.z;
	return 1.0 - PCFDirectionalLight(shadowMap, shadowCoords, uvRadius) * ShadowFade;
}

/////////////////////////////////////////////

void main()
{
	// Standard PBR inputs
	mParams.Albedo = uAlbedoTexToggle > 0.5 ? texture(uAlbedoTexture, vsInput.TexCoord).rgb : uAlbedoColor; 
	mParams.Metalness = uMetalnessTexToggle > 0.5 ? texture(uMetalnessTexture, vsInput.TexCoord).r : uMetalness;
	mParams.Roughness = uRoughnessTexToggle > 0.5 ?  texture(uRoughnessTexture, vsInput.TexCoord).r : uRoughness;
    mParams.Roughness = max(mParams.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight

	// Normals (either from vertex or map)
	mParams.Normal = normalize(vsInput.Normal);
	if (uNormalTexToggle > 0.5)
	{
		mParams.Normal = normalize(2.0 * texture(uNormalTexture, vsInput.TexCoord).rgb - 1.0);
		mParams.Normal = normalize(vsInput.WorldNormals * mParams.Normal);
	}

	mParams.View = normalize(uCameraPosition - vsInput.WorldPosition);
	mParams.NdotV = max(dot(mParams.Normal, mParams.View), 0.0);
		
	// Specular reflection vector
	vec3 Lr = 2.0 * mParams.NdotV * mParams.Normal - mParams.View;

	// Fresnel reflectance, metals use albedo
	vec3 F0 = mix(Fdielectric, mParams.Albedo, mParams.Metalness);

	const uint SHADOWMAPCASCADECOUNT = 4;
	for(uint i = 0; i < SHADOWMAPCASCADECOUNT - 1; ++i)
	{
		if(vsInput.ViewPosition.z < uCascadeSplits[i])
		{
			CascadeIndex = i + 1;
		}
	}

	float shadowDistance = uMaxShadowDistance;//uCascadeSplits[3];
	float transitionDistance = uShadowFade;
	float distance = length(vsInput.ViewPosition);
	ShadowFade = distance - (shadowDistance - transitionDistance);
	ShadowFade /= transitionDistance;
	ShadowFade = clamp(1.0 - ShadowFade, 0.0, 1.0);

	bool fadeCascades = uCascadeFading;
	float shadowAmount = 1.0;
	if (fadeCascades)
	{
		float cascadeTransitionFade = uCascadeTransitionFade;
		
		float c0 = smoothstep(uCascadeSplits[0] + cascadeTransitionFade * 0.5f, uCascadeSplits[0] - cascadeTransitionFade * 0.5f, vsInput.ViewPosition.z);
		float c1 = smoothstep(uCascadeSplits[1] + cascadeTransitionFade * 0.5f, uCascadeSplits[1] - cascadeTransitionFade * 0.5f, vsInput.ViewPosition.z);
		float c2 = smoothstep(uCascadeSplits[2] + cascadeTransitionFade * 0.5f, uCascadeSplits[2] - cascadeTransitionFade * 0.5f, vsInput.ViewPosition.z);
		if (c0 > 0.0 && c0 < 1.0)
		{
			// Sample 0 & 1
			vec3 shadowMapCoords = (vsInput.ShadowMapCoords[0].xyz / vsInput.ShadowMapCoords[0].w);
			float shadowAmount0 = uSoftShadows ? PCSSDirectionalLight(uShadowMapTexture[0], shadowMapCoords, uLightSize) : HardShadowsDirectionalLight(uShadowMapTexture[0], shadowMapCoords);
			shadowMapCoords = (vsInput.ShadowMapCoords[1].xyz / vsInput.ShadowMapCoords[1].w);
			float shadowAmount1 = uSoftShadows ? PCSSDirectionalLight(uShadowMapTexture[1], shadowMapCoords, uLightSize) : HardShadowsDirectionalLight(uShadowMapTexture[1], shadowMapCoords);

			shadowAmount = mix(shadowAmount0, shadowAmount1, c0);
		}
		else if (c1 > 0.0 && c1 < 1.0)
		{
			// Sample 1 & 2
			vec3 shadowMapCoords = (vsInput.ShadowMapCoords[1].xyz / vsInput.ShadowMapCoords[1].w);
			float shadowAmount1 = uSoftShadows ? PCSSDirectionalLight(uShadowMapTexture[1], shadowMapCoords, uLightSize) : HardShadowsDirectionalLight(uShadowMapTexture[1], shadowMapCoords);
			shadowMapCoords = (vsInput.ShadowMapCoords[2].xyz / vsInput.ShadowMapCoords[2].w);
			float shadowAmount2 = uSoftShadows ? PCSSDirectionalLight(uShadowMapTexture[2], shadowMapCoords, uLightSize) : HardShadowsDirectionalLight(uShadowMapTexture[2], shadowMapCoords);

			shadowAmount = mix(shadowAmount1, shadowAmount2, c1);
		}
		else if (c2 > 0.0 && c2 < 1.0)
		{
			// Sample 2 & 3
			vec3 shadowMapCoords = (vsInput.ShadowMapCoords[2].xyz / vsInput.ShadowMapCoords[2].w);
			float shadowAmount2 = uSoftShadows ? PCSSDirectionalLight(uShadowMapTexture[2], shadowMapCoords, uLightSize) : HardShadowsDirectionalLight(uShadowMapTexture[2], shadowMapCoords);
			shadowMapCoords = (vsInput.ShadowMapCoords[3].xyz / vsInput.ShadowMapCoords[3].w);
			float shadowAmount3 = uSoftShadows ? PCSSDirectionalLight(uShadowMapTexture[3], shadowMapCoords, uLightSize) : HardShadowsDirectionalLight(uShadowMapTexture[3], shadowMapCoords);

			shadowAmount = mix(shadowAmount2, shadowAmount3, c2);
		}
		else
		{
			vec3 shadowMapCoords = (vsInput.ShadowMapCoords[CascadeIndex].xyz / vsInput.ShadowMapCoords[CascadeIndex].w);
			shadowAmount = uSoftShadows ? PCSSDirectionalLight(uShadowMapTexture[CascadeIndex], shadowMapCoords, uLightSize) : HardShadowsDirectionalLight(uShadowMapTexture[CascadeIndex], shadowMapCoords);
		}
	}
	else
	{
		vec3 shadowMapCoords = (vsInput.ShadowMapCoords[CascadeIndex].xyz / vsInput.ShadowMapCoords[CascadeIndex].w);
		shadowAmount = uSoftShadows ? PCSSDirectionalLight(uShadowMapTexture[CascadeIndex], shadowMapCoords, uLightSize) : HardShadowsDirectionalLight(uShadowMapTexture[CascadeIndex], shadowMapCoords);
	}

	float NdotL = dot(mParams.Normal, uDirectionalLights.Direction);
	NdotL = smoothstep(0.0, 0.4, NdotL + 0.2);
	shadowAmount *= (NdotL * 1.0);

	vec3 iblContribution = IBL(F0, Lr) * uIBLContribution;
	vec3 lightContribution = uDirectionalLights.Multiplier > 0.0f ? (Lighting(F0) * shadowAmount) : vec3(0.0f);

	color = vec4(lightContribution + iblContribution, 1.0);

	// Bloom
	float brightness = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
	oBloomColor = vec4(0.0, 0.0, 0.0, 1.0);
	if (brightness > uBloomThreshold)
	{
		oBloomColor = color;
	}

	if (uShowCascades)
	{
		switch(CascadeIndex)
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