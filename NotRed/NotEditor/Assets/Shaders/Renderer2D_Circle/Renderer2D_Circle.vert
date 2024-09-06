#version 450 core

layout(location = 0) in vec3 aWorldPosition;
layout(location = 1) in float aThickness;
layout(location = 2) in vec2 aLocalPosition;
layout(location = 3) in vec4 aColor;
uniform mat4 uViewProjection;

out vec2 vLocalPosition;
out float vThickness;
out vec4 vColor;

void main()
{
	vLocalPosition = aLocalPosition;
	vThickness = aThickness;
	vColor = aColor;
	gl_Position = uViewProjection * vec4(aWorldPosition, 1.0);
}