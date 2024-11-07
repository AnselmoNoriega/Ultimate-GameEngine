#version 450 core

layout(location = 0) in vec2 a_texPos;
layout(location = 1) in vec4 a_color;
layout(location = 2) in flat uint a_type;

/////////////////////////////////////////////

layout(location = 0) out vec4 o_color;

layout(push_constant) uniform Galaxy
{
	layout(offset = 64) vec3 StarColor;
	vec3 DustColor;
	vec3 h2RegionColor;
} uGalaxySpecs;

void main() 
{
	vec2 centeredPos = 2.0 * (a_texPos - 0.5);
	
	vec4 color = a_color;
	if(a_type == 0) //stars
	{
		if(dot(centeredPos, centeredPos) > 1.0)
			discard;

		color.rgb *= uGalaxySpecs.StarColor;
		color = clamp(color, 0.0, 4.0);
	}
	else if(a_type == 1) //dust
	{
		color = clamp(color, 0.0, 1.0);
		color.rgb *= uGalaxySpecs.DustColor;
		color.a *= max(1.0 - length(centeredPos), 0.0) * 0.1;

		//clamp(color, 0.0, 1.0);
	}
	else //h-2 regions
	{
		float dist = max(1.0 - length(centeredPos), 0.0);

		color.rgb = mix(uGalaxySpecs.h2RegionColor, vec3(1.0), dist * dist * dist);
		color.a = dist * dist * 4;

		color = clamp(color, 0.0, 4.0);
	}

	o_color = color;
}