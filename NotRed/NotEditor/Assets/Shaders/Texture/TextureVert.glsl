#version 330 core
        
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aTexIndex;
layout(location = 4) in int aEntityID;

uniform mat4 uViewProjection;

out vec2 vTexCoord;
out vec4 vColor;
out float vTexIndex;
flat out int vEntityID;

void main()
{
    vTexCoord = aTexCoord;
    vColor = aColor;
    vTexIndex = aTexIndex;
    vEntityID = aEntityID;
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
}