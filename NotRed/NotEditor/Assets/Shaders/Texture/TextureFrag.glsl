#version 330 core

uniform sampler2D uTextures[32];

in vec2 vTexCoord;
in vec4 vColor;
in float vTexIndex;

out vec4 color;

void main()
{
    color = texture(uTextures[int(vTexIndex)], vTexCoord) * vColor;
}