#version 450 core

layout(location = 0) out vec4 oColor;

in vec2 vTexCoord;

uniform sampler2D uSceneTexture;
uniform sampler2D uBloomTexture;

uniform float uExposure;
uniform bool uEnableBloom;

void main()
{
#if 1
	const float gamma     = 2.2;
	const float pureWhite = 1.0;

    // Tonemapping
	vec3 color = texture(uSceneTexture, vTexCoord).rgb;
	if (uEnableBloom)
	{
		vec3 bloomColor = texture(uBloomTexture, vTexCoord).rgb;
		color += bloomColor;
	}

	// Reinhard tonemapping
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color* uExposure;

	// Gamma correction.
	oColor = vec4(color, 1.0);
#else
	const float gamma = 2.2;
    vec3 hdrColor = texture(uSceneTexture, vTexCoord).rgb;      
    vec3 bloomColor = texture(uBloomTexture, vTexCoord).rgb;
    hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * uExposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));
    oColor = vec4(result, 1.0);
#endif
}