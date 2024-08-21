#version 450 core

layout(location = 0) out vec4 oColor;

in vec2 vTexCoord;

uniform sampler2DMS uTexture;
uniform float uExposure;
uniform int uTextureSamples;

vec4 MultiSampleTexture(sampler2DMS tex, ivec2 texCoord, int samples)
{
    vec4 result = vec4(0.0);
    for (int i = 0; i < samples; i++)
        {
			result += texelFetch(tex, texCoord, i);
		}
		
    result /= float(samples);
    return result;
}

void main()
{
	const float gamma     = 2.2;
	const float pureWhite = 1.0;
	
	ivec2 texSize = textureSize(uTexture);
	ivec2 texCoord = ivec2(vTexCoord * texSize);
	vec4 msColor = MultiSampleTexture(uTexture, texCoord, uTextureSamples);

	vec3 color = msColor.rgb * uExposure;
	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	oColor  = vec4(pow(mappedColor, vec3(1.0/gamma)), 1.0);
}