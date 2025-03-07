#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout(location = 0) out vec2 vs_TexCoords;

void main()
{
    vs_TexCoords = aTexCoord;
    gl_Position = vec4(aPosition.xy, 0.0, 1.0);
}