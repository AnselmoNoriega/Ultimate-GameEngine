#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 uViewProjection;
uniform mat4 uTransform;

out vec2 vTexCoord;

void main()
{
	gl_Position = uViewProjection * uTransform * vec4(aPosition, 1.0);
}