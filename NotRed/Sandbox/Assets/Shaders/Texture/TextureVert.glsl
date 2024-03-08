#version 330 core
        
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aTexIndex;

uniform mat4 uViewProjection;

out vec2 vTexCoord;
out vec4 vColor;
out float vTexIndex;

void main()
{
    vTexCoord = aTexCoord;
    vColor = aColor;
    vTexIndex = aTexIndex;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}