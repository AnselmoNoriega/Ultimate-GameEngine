#version 450 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 unused;

#ifdef OPENGL
layout (std140, binding = 1) uniform Settings {
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
	unused = vec4(0.0);
}