#version 450 core

layout(location = 0) out vec4 aColor;

uniform sampler2D uTexture;
uniform float uScale;
uniform float uRes;

in vec2 vTexCoord;

float grid(vec2 st, float res)
{
	vec2 grid = fract(st);
	return step(res, grid.x) * step(res, grid.y);
}

void main()
{
	float scale = uScale;
	float resolution = uRes;

	float x = grid(vTexCoord * scale, resolution);
	aColor = vec4(vec3(0.2), 0.5) * (1.0 - x);
}