#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 uInverseVP;

out vec3 vPosition;

void main()
{
	vec4 position = vec4(aPosition.xy, 1.0, 1.0);
	gl_Position = position;

	vPosition = (uInverseVP * position).xyz;
}