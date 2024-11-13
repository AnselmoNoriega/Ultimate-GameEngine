#version 450 core

layout(location = 0) out vec4 oColor;

struct VertexOutput
{
	vec2 TexCoords;
};

layout (location = 0) in VertexOutput Input;

layout(binding = 0) uniform sampler2D uTexture;

void main()
{
    vec4 pixel = texture(uTexture, Input.TexCoords);

    // Signed distance (squared)
    float dist = sqrt(pixel.z);
    float alpha = smoothstep(0.004f, 0.002f, dist);

    if (alpha == 0.0)
        discard;

    vec3 outlineColor = vec3(0.949f, 0.0f, 0.0f);
    oColor = vec4(outlineColor, alpha);
}