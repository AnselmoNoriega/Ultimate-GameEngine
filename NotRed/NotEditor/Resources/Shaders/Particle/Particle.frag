#version 450 core

layout(location = 0) in vec2 a_texPos;
layout(location = 1) in vec4 a_color;
layout(location = 2) in flat uint a_type;

/////////////////////////////////////////////

layout(location = 0) out vec4 o_color;

void main() 
{
    o_color = vec4(0.0, 1.0, 0.0, 1.0);
}