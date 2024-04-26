#version 450 core

layout(location = 0) out vec4 oColor;
layout(location = 1) out int oEntityID;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
};

layout (location = 0) in VertexOutput Input;
layout (location = 2) in flat int vEntityID;

layout (binding = 0) uniform sampler2D uFontAtlas;

float screenPxRange() 
{
	const float pxDistance = 2.0;
    vec2 unitRange = vec2(pxDistance) / vec2(textureSize(uFontAtlas, 0));
    vec2 screenTexSize = vec2(1.0) / fwidth(Input.TexCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

float median(float r, float g, float b) 
{
    return max(min(r, g), min(max(r, g), b));
}

void main()
{
	vec4 texColor = Input.Color * texture(uFontAtlas, Input.TexCoord);

	vec3 msd = texture(uFontAtlas, Input.TexCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange() * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
	if (opacity == 0.0)
	{
		discard;
	}

	vec4 bgColor = vec4(0.0);
    oColor = mix(bgColor, Input.Color, opacity);
	if (oColor.a == 0.0)
	{
		discard;
	}

	oEntityID = vEntityID;
}