
cbuffer matrix_cb : register(b0) {
    float4x4 g_MODEL;
    float4x4 g_VIEW;
    float4x4 g_PROJECTION;
};

cbuffer time_cb : register(b1) {
    float g_GAME_TIME;
    float g_SYSTEM_TIME;
    float g_GAME_FRAME_TIME;
    float g_SYSTEM_FRAME_TIME;
}

cbuffer fullscreen_cb : register(b3) {
    int g_effectIndex;
    float g_fadePercent;
    float2 g_PADDING;
    float4 g_fadeColor;
}

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : UV;
    uint id : SV_VertexID;
};

struct ps_in_t {
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

SamplerState sSampler : register(s0);

Texture2D<float4> tDiffuse    : register(t0);
Texture2D<float4> tNormal   : register(t1);
Texture2D<float4> tDisplacement : register(t2);
Texture2D<float4> tSpecular : register(t3);
Texture2D<float4> tOcclusion : register(t4);
Texture2D<float4> tEmissive : register(t5);


ps_in_t VertexFunction(uint id : SV_VertexID) {
    ps_in_t output;
    output.color = float4(1.0f, 1.0f, 1.0f, 1.0f);

    output.uv.x = (float)(id / 2) * 2.0f;
    output.uv.y = 1.0f - (float)(id % 2) * 2.0f;
    output.position.x = (float)(id / 2) * 4.0f - 1.0f;
    output.position.y = (float)(id % 2) * 4.0f - 1.0f;
    output.position.z = 0.0f;
    output.position.w = 1.0f;

    return output;
}

float4 PixelFunction(ps_in_t input) : SV_Target0
{
    float4 albedo = tDiffuse.Sample(sSampler, input.uv);
    float4 diffuse = albedo * input.color;
    float4 fadeColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    switch(g_effectIndex) {
    case 0:
        return lerp(g_fadeColor, diffuse, g_fadePercent);
    case 1:
        return lerp(g_fadeColor, diffuse, 1.0f - g_fadePercent);
    default:
        return diffuse;
    }
}