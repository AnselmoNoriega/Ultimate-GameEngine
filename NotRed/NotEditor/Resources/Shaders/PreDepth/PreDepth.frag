#version 450 core

layout(location = 0) out vec4 oLinearDepth;
layout(location = 0) in float LinearDepth;

void main()
{
	oLinearDepth = vec4(LinearDepth, 0.0, 0.0, 1.0);
}