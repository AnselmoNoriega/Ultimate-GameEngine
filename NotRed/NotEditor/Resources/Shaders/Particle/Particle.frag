#version 450 core

layout(location = 16) in vec4 fragColor;
layout(location = 0) out vec4 color;

/////////////////////////////////////////////

void main() 
{
    color = fragColor;  // Red color
}