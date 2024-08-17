#version 450 core

in vec2 vTexCoord;

uniform sampler2D uTexture;

layout(location=0) out vec4 aOutColor;

uniform float uExposure;

void main()
{
	const float gamma     = 2.2;
	const float pureWhite = 1.0;

	vec3 color = texture(uTexture, vTexCoord).rgb * uExposure;
	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	aOutColor = vec4(pow(mappedColor, vec3(1.0/gamma)), 1.0);
}