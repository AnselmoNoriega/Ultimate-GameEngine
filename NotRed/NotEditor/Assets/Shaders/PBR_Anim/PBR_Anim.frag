#version 450 core

const float PI = 3.141592;
const float Epsilon = 0.00001;

const int LightCount = 1;

// Constant normal incidence Fresnel factor for all dielectrics.
const vec3 Fdielectric = vec3(0.04);

struct Light {
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
	vec3 Binormal;
} vsInput;

layout(location = 0) out vec4 color;

uniform Light lights;
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

uniform vec3 uAlbedoColor;
uniform float uMetalness;
uniform float uRoughness;

uniform float uEnvMapRotation;

// Toggles
uniform float uRadiancePrefilter;
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
	for(int i = 0; i < NumSamples; i++)
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
	for(int i = 0; i < LightCount; i++)
	{
		vec3 Li = -lights.Direction;
		vec3 Lradiance = lights.Radiance * lights.Multiplier;
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
	
	vec3 lightContribution = Lighting(F0);
	vec3 iblContribution = IBL(F0, Lr);

	color = vec4(lightContribution + iblContribution, 1.0);
}