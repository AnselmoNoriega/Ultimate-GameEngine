#version 450 core

layout(location = 0) in vec2 a_texPos;
layout(location = 1) in vec4 a_color;
layout(location = 2) in flat uint a_type;

/////////////////////////////////////////////

layout(location = 0) out vec4 o_color;

void main() 
{
	vec2 centeredPos = 2.0 * (a_texPos - 0.5);
	
	vec4 color = a_color;
	if(a_type == 0) //stars
	{
		color = vec4(0, 0, 1, 1);
	}
	else if(a_type == 1) //dust
	{
		color = vec4(0, 1, 0, 1);
	}
	else //h-2 regions
	{
		float dist = max(1.0 - length(centeredPos), 0.0);

		color = vec4(1, 0, 0, 1);
	}

	o_color = color;
}