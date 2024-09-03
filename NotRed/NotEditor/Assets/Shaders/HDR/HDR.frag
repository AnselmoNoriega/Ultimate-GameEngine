#version 450 core

layout(location = 0) out vec4 oColor;
layout(location = 1) out vec4 oBloomTexture;

in vec2 vTexCoord;

uniform sampler2DMS uTexture;

uniform float uExposure;
uniform int uTextureSamples;

uniform bool uEnableBloom;
uniform float uBloomThreshold;

const float uFar = 1.0;

vec4 SampleTexture(sampler2D tex, vec2 texCoord)
{
    return texture(tex, texCoord);
}

vec4 MultiSampleTexture(sampler2DMS tex, vec2 tc)
{
	ivec2 texSize = textureSize(tex);
	ivec2 texCoord = ivec2(tc * texSize);
	vec4 result = vec4(0.0);
    for (int i = 0; i < uTextureSamples; ++i)
    {
		result += texelFetch(tex, texCoord, i);
	}
		
    result /= float(uTextureSamples);
    return result;
}

void main()
{
	const float gamma     = 2.2;
	const float pureWhite = 1.0;
	
	vec4 msColor = MultiSampleTexture(uTexture, vTexCoord);

	vec3 color = msColor.rgb;

	if (uEnableBloom)
	{
		vec3 bloomColor = MultiSampleTexture(uTexture, vTexCoord).rgb;
		color += bloomColor;
	}

	color *= uExposure;

	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	oColor  = vec4(pow(mappedColor, vec3(1.0/gamma)), 1.0);
}