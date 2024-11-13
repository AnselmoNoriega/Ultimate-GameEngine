#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 unused0;
layout(location = 2) out vec4 unused1;

#ifdef OPENGL

layout (std140, binding = 8) uniform Settings {
	layout (offset = 64) float Scale;
	float Size;
} uSettings;

#else

layout (push_constant) uniform Settings
{
	layout (offset = 64) float Scale;
	float Size;
} uSettings;

#endif

layout (location = 0) in vec2 vTexCoord;

float grid(vec2 st, float res)
{
	vec2 grid = fract(st);
	return step(res, grid.x) * step(res, grid.y);
}

void main()
{
	float x = grid(vTexCoord * uSettings.Scale, uSettings.Size);
	color = vec4(vec3(0.2), 0.5) * (1.0 - x);
	if (color.a == 0.0)
	{
		discard;
	}
	unused0 = vec4(0.0);
	unused1 = vec4(0.0);
}