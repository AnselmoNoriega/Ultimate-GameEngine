#version 450 core

layout(location = 0) out vec4 oColor;

struct OutputBlock
{
	vec2 TexCoord;
};

layout (location = 0) in OutputBlock Input;
layout (binding = 0) uniform sampler2D uTexture;
layout (binding = 1) uniform sampler2D uDepthTexture;

layout(push_constant) uniform Uniforms
{
	float Exposure;
} uUniforms;

// WIP depth of field from https://blog.tuxedolabs.com/2018/05/04/bokeh-depth-of-field-in-single-pass.html
const float GOLDEN_ANGLE = 2.39996323;
const float MAX_BLUR_SIZE = 20.0;
const float RAD_SCALE = 1.0; // Smaller = nicer blur, larger = faster
const float uFar = 1000.0;

float LinearizeDepth(float d, float zNear, float zFar)
{
	return zNear * zFar / (zFar + d * (zNear - zFar));
}

float GetBlurSize(float depth, float focusPoint, float focusScale)
{
	float coc = clamp((1.0 / focusPoint - 1.0 / depth) * focusScale, -1.0, 1.0);
	return abs(coc) * MAX_BLUR_SIZE;
}

vec3 DepthOfField(vec2 texCoord, float focusPoint, float focusScale, vec2 texelSize)
{
	float centerDepth = LinearizeDepth(texture(uDepthTexture, texCoord).r, 0.1, uFar);// *uFar;
	centerDepth = texture(uDepthTexture, texCoord).r * uFar;
	
	float centerSize = GetBlurSize(centerDepth, focusPoint, focusScale);
	vec3 color = texture(uTexture, texCoord).rgb;
	float tot = 1.0;
	float radius = RAD_SCALE;
	
	for (float ang = 0.0; radius < MAX_BLUR_SIZE; ang += GOLDEN_ANGLE)
	{
		vec2 tc = texCoord + vec2(cos(ang), sin(ang)) * texelSize * radius;
		vec3 sampleColor = texture(uTexture, tc).rgb;
		float sampleDepth = texture(uDepthTexture, tc).r * uFar;
		float sampleSize = GetBlurSize(sampleDepth, focusPoint, focusScale);
		
		if (sampleDepth > centerDepth)
			sampleSize = clamp(sampleSize, 0.0, centerSize * 2.0);

		float m = smoothstep(radius - 0.5, radius + 0.5, sampleSize);
		color += mix(color / tot, sampleColor, m);
		tot += 1.0;
		radius += RAD_SCALE / radius;
	}

	return color /= tot;
}

void main()
{
	ivec2 texSize = textureSize(uTexture, 0);
	vec2 fTexSize = vec2(float(texSize.x), float(texSize.y));
	float centerDepth = LinearizeDepth(texture(uDepthTexture, vec2(0.5f)).r, 0.1, uFar);// *uFar;
	
	//centerDepth = texture(uDepthTexture, vec2(0.5f)).r;
	float focusPoint = centerDepth;
	float focusScale = 0.8f;
	vec3 color = DepthOfField(Input.TexCoord, focusPoint, focusScale, 1.0f / fTexSize);
	
	oColor = vec4(color, 1.0);
	
	if (length(vec2(0.5) - Input.TexCoord) < 0.05)
	{
		oColor.rgb = vec3(texture(uDepthTexture, vec2(0.5f)).r);
	}
}