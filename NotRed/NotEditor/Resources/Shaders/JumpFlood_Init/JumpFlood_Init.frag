#version 450 core

layout(location = 0) out vec4 outColor;

struct VertexOutput
{
	vec2 TexCoords;
};

layout (location = 0) in VertexOutput Input;
layout(binding = 0) uniform sampler2D uTexture;

float ScreenDistance(vec2 v, vec2 texelSize)
{
    float ratio = texelSize.x / texelSize.y;
    v.x /= ratio;
    return dot(v, v);
}

void main()
{
    vec4 color = texture(uTexture, Input.TexCoords);
    ivec2 texSize = textureSize(uTexture, 0);
    vec2 texelSize = vec2(1.0f / float(texSize.x), 1.0f / float(texSize.y));
    vec4 result;

    result.xy = vec2(100, 100);
    result.z = ScreenDistance(result.xy, texelSize);

    // Inside vs. outside
    result.w = color.a > 0.5f ? 1.0f : 0.0f;
    outColor = result;
}