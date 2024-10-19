#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoords;

struct VertexOutput
{
	vec2 TexCoords;
    vec2 TexelSize;
    vec2 UV[9];
};

layout (location = 0) out VertexOutput Output;
layout(push_constant) uniform Uniforms
{
    vec2 TexelSize;
    int Step;
} uRenderer;

void main()
{
    Output.TexCoords = aTexCoords;
    Output.TexelSize = uRenderer.TexelSize;

    vec2 dx = vec2(uRenderer.TexelSize.x, 0.0f) * uRenderer.Step;
    vec2 dy = vec2(0.0f, uRenderer.TexelSize.y) * uRenderer.Step;

    Output.UV[0] = Output.TexCoords;
    
    //Sample all pixels within a 3x3 block
    Output.UV[1] = Output.TexCoords + dx;
    Output.UV[2] = Output.TexCoords - dx;
    Output.UV[3] = Output.TexCoords + dy;
    Output.UV[4] = Output.TexCoords - dy;
    Output.UV[5] = Output.TexCoords + dx + dy;
    Output.UV[6] = Output.TexCoords + dx - dy;
    Output.UV[7] = Output.TexCoords - dx + dy;
    Output.UV[8] = Output.TexCoords - dx - dy;
   
    gl_Position = vec4(aPosition, 1.0f);
}