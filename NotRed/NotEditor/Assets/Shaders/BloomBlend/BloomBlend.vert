#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
	vec4 position = vec4(aPosition.xy, 0.0, 1.0);
	vTexCoord = aTexCoord;
	gl_Position = position;
}