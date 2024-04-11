#version 450 core

layout(location = 0) out vec4 oColor;
layout(location = 1) out int oEntityID;

struct VertexOutput
{
	vec3 LocalPosition;
	vec4 Color;
	float Thickness;
};

layout (location = 0) in VertexOutput Input;
layout (location = 3) in flat int vEntityID;

void main()
{
	float fade = 0.005f;
    float disFromCenter = 1.0 - length(Input.LocalPosition);
    float circle = smoothstep(0.0, fade, disFromCenter);
    circle *= smoothstep(Input.Thickness + fade, Input.Thickness, disFromCenter);

	if (circle == 0.0)
	{
		discard;
	}

    oColor = Input.Color;
	oColor.a *= circle;

	oEntityID = vEntityID;
}