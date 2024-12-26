#version 450

layout(binding = 0) uniform sampler2D uLinearDepthTex;

layout(std140, binding = 16) uniform ScreenData
{
    vec2 uInvFullResolution;
    vec2 uFullResolution;
};

layout(push_constant) uniform Info
{
    vec2 UVOffset;
} uInfo;

layout(location = 0) out vec4 out_Color[8];

//----------------------------------------------------------------------------------
#if 1
void main() 
{
    vec2 uv = floor(gl_FragCoord.xy) * 4.0 + uInfo.UVOffset + 0.5;
    uv *= uInvFullResolution;

    vec4 S0 = textureGather(uLinearDepthTex, uv, 0);
    vec4 S1 = textureGatherOffset(uLinearDepthTex, uv, ivec2(2, 0), 0);

    out_Color[0] = vec4(S0.w, 0.0, 0.0, 1.0);
    out_Color[1] = vec4(S0.z, 0.0, 0.0, 1.0);
    out_Color[2] = vec4(S1.w, 0.0, 0.0, 1.0);
    out_Color[3] = vec4(S1.z, 0.0, 0.0, 1.0);
    out_Color[4] = vec4(S0.x, 0.0, 0.0, 1.0);
    out_Color[5] = vec4(S0.y, 0.0, 0.0, 1.0);
    out_Color[6] = vec4(S1.x, 0.0, 0.0, 1.0);
    out_Color[7] = vec4(S1.y, 0.0, 0.0, 1.0);
}

#else
void main() 
{
    vec2 uv = floor(gl_FragCoord.xy) * 4.0 + uInfo.UVOffset;
    ivec2 tc = ivec2(uv);

    out_Color[0] = texelFetchOffset(uLinearDepthTex, tc, 0, ivec2(0, 0)).x;
    out_Color[1] = texelFetchOffset(uLinearDepthTex, tc, 0, ivec2(1, 0)).x;
    out_Color[2] = texelFetchOffset(uLinearDepthTex, tc, 0, ivec2(2, 0)).x;
    out_Color[3] = texelFetchOffset(uLinearDepthTex, tc, 0, ivec2(3, 0)).x;
    out_Color[4] = texelFetchOffset(uLinearDepthTex, tc, 0, ivec2(0, 1)).x;
    out_Color[5] = texelFetchOffset(uLinearDepthTex, tc, 0, ivec2(1, 1)).x;
    out_Color[6] = texelFetchOffset(uLinearDepthTex, tc, 0, ivec2(2, 1)).x;
    out_Color[7] = texelFetchOffset(uLinearDepthTex, tc, 0, ivec2(3, 1)).x;
}

#endif