#version 330 core

uniform sampler2D uTexture;
uniform vec4 uColor;

in vec2 vTexCoord;

out vec4 color;

void main()
{
    color = texture(uTexture, vTexCoord) * uColor;
}