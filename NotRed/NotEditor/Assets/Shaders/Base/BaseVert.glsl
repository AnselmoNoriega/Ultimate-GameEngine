#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBitangent;
layout(location = 4) in vec2 aTexCoord;

uniform mat4 uMVP;

out vec3 vNormal;

void main()
{
	gl_Position = uMVP * vec4(aPosition, 1.0);
	vNormal = aNormal;
}