#version 450 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uViewProjection;
uniform mat4 uTransform;

void main()
{
	gl_Position = uViewProjection * uTransform * vec4(aPosition, 1.0);
}