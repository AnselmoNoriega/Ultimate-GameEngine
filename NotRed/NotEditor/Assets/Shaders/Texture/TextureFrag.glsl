#version 330 core

layout(location = 0) out vec4 color;
layout(location = 1) out int entityID;

uniform sampler2D uTextures[32];

in vec2 vTexCoord;
in vec4 vColor;
in float vTexIndex;
flat in int vEntityID;

void main()
{
    color = texture(uTextures[int(vTexIndex)], vTexCoord) * vColor;
    entityID = vEntityID;
}