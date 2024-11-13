#version 450 core

layout(location = 0) out vec4 color;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
	float TilingFactor;
};

layout (location = 0) in VertexOutput Input;

layout (binding = 1) uniform sampler2D uTextures[32];
layout (location = 5) in flat float TexIndex;

void main()
{
	color = texture(uTextures[int(TexIndex)], Input.TexCoord * Input.TilingFactor) * Input.Color;
	if (color.a < 0.5)
		discard;
}