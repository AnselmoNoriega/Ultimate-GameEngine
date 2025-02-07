#version 450 core

layout(location = 0) out vec4 color;
struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
};

layout (location = 0) in VertexOutput Input;
layout (location = 5) in flat float TexIndex;
layout (binding = 1) uniform sampler2D uFontAtlases[32];

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float ScreenPxRange()
{
	float pxRange = 1.2f;
    vec2 unitRange = vec2(pxRange)/vec2(textureSize(uFontAtlases[int(TexIndex)], 0));
    vec2 screenTexSize = vec2(1.0)/fwidth(Input.TexCoord);
    return max(0.5*dot(unitRange, screenTexSize), 1.0);
}

void main()
{
	vec4 bgColor = vec4(0.0);
	vec4 fgColor = Input.Color;

	vec3 msd = texture(uFontAtlases[int(TexIndex)], Input.TexCoord).rgb;

    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = ScreenPxRange()*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    color = mix(bgColor, fgColor, opacity);
	
	if (opacity == 0.0)
	{
		discard;
	}
}