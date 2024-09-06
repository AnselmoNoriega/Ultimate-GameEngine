#version 450 core

layout(location = 0) out vec4 color;

in vec2 vLocalPosition;
in float vThickness;
in vec4 vColor;

void main()
{
	float fade = 0.01;
	float dist = sqrt(dot(vLocalPosition, vLocalPosition));
	if (dist > 1.0 || dist < 1.0 - vThickness - fade)
	{
		discard;
	}

	float alpha = 1.0 - smoothstep(1.0f - fade, 1.0f, dist);
	alpha *= smoothstep(1.0 - vThickness - fade, 1.0 - vThickness, dist);
	color = vColor;
	color.a = alpha;
}