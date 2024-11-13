#version 450 core

layout(location = 0) out vec4 oColor;

struct VertexOutput
{
	vec2 TexCoords;
    vec2 TexelSize;
    vec2 UV[9];
};

layout (location = 0) in VertexOutput Input;
layout(binding = 0) uniform sampler2D uTexture;

float ScreenDistance(vec2 v)
{
    float ratio = Input.TexelSize.x / Input.TexelSize.y;
    v.x /= ratio;
    return dot(v, v);
}

void BoundsCheck(inout vec2 xy, vec2 uv)
{
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
        xy = vec2(1000.0f);
}

void main()
{
    vec4 pixel = texture(uTexture, Input.UV[0]);

    for (int j = 1; j <= 8; ++j)
    {
        // Sample neighbouring pixel and make sure it's
        // on the same side as us
        vec4 n = texture(uTexture, Input.UV[j]);
        if (n.w != pixel.w)
            n.xyz = vec3(0.0f);

        n.xy += Input.UV[j] - Input.UV[0];

        // Invalidate out of bounds neighbours
        BoundsCheck(n.xy, Input.UV[j]);
        float dist = ScreenDistance(n.xy);
        if (dist < pixel.z)
            pixel.xyz = vec3(n.xy, dist);
    }

    oColor = pixel;
}