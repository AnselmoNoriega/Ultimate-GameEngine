#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 4) in vec2 aTexCoord;

uniform mat4 uMVP;

out vec2 vTexCoord;

void main()
{
	vec4 position = uMVP * vec4(aPosition, 1.0);
	gl_Position = position;

	vTexCoord = aTexCoord;
}
