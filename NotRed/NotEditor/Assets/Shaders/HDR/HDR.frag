#version 450 core

layout(location = 0) out vec4 oColor;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) in OutputBlock Input;

layout (binding = 0) uniform sampler2D uTexture;

layout(push_constant) uniform Uniforms
{
	float Exposure;
} uUniforms;

void main()
{
	const float gamma     = 2.2;
	const float pureWhite = 1.0;

	vec3 color = texture(uTexture, Input.TexCoord).rgb * uUniforms.Exposure;
	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance / (pureWhite * pureWhite))) / (1.0 + luminance);

	// Scale color by ratio of average luminances.
	vec3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction.
	oColor = vec4(pow(mappedColor, vec3(1.0 / gamma)), 1.0);
}