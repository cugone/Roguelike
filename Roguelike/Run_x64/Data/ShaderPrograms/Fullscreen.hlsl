
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
    float g_greyscaleBrightness;
    float g_gradRadius;
    float4 g_fadeColor;
    float4 g_gradColor;
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

float4 FadeIn(float4 color, float4 diffuse, float percent) {
    return lerp(color, diffuse, percent);
}

float4 FadeOut(float4 color, float4 diffuse, float percent) {
    return lerp(color, diffuse, 1.0f - percent);
}

float GetLinearIntensity(float color) {
    float result;
    float LUM_BREAKPOINT = 0.04045f;
    color = saturate(color);
    if(color > LUM_BREAKPOINT) {
        result = pow((color + 0.055f) / 1.055f, g_greyscaleBrightness);
    } else {
        result = color / 12.92f;
    }
    return result;
}

float4 Lumosity(float4 color) {
    float lin_r = GetLinearIntensity(color.r);
    float lin_g = GetLinearIntensity(color.g);
    float lin_b = GetLinearIntensity(color.b);

    float3 a = float3(0.2126f, 0.7152f, 0.0722f);
    float3 b = float3(lin_r, lin_g, lin_b);
    float l = dot(a, b);
    return float4(l, l, l, 1.0f);
}

float4 Sepia(float4 color) {
    float3x3 sepia_transform = float3x3(
        float3(0.393f, 0.349f, 0.272f),
        float3(0.769f, 0.686f, 0.534f),
        float3(0.189f, 0.168f, 0.131f)
        );
    float3 sepia = mul(color.xyz, sepia_transform);

    float4 final_color = float4(sepia, color.a);
    return final_color;
}

float4 CircularGradient(float2 uv, float4 diffuse, float radius, float4 color) {
    float2 D = distance(float2(0.5f, 0.5f), uv);
    return lerp(diffuse, color, saturate(D).x);
}

float4 SquareBlur(float2 uv, float w, float h, float4 diffuse) {
    float px = 1.0f / w;
    float py = 1.0f / h;

    float2 tl = float2(uv.x - px, uv.y - py);
    float2 tm = float2(uv.x, uv.y - py);
    float2 tr = float2(uv.x + px, uv.y - py);
    float2 cl = float2(uv.x - px, uv.y);
    float2 cm = float2(uv.x, uv.y);
    float2 cr = float2(uv.x + px, uv.y);
    float2 bl = float2(uv.x - px, uv.y + py);
    float2 bm = float2(uv.x, uv.y + py);
    float2 br = float2(uv.x + px, uv.y + py);

    float4 top_sum = tDiffuse.Sample(sSampler, tl) + tDiffuse.Sample(sSampler, tm) + tDiffuse.Sample(sSampler, tr);
    float4 middle_sum = tDiffuse.Sample(sSampler, cl) + tDiffuse.Sample(sSampler, cm) + tDiffuse.Sample(sSampler, cr);
    float4 bottom_sum = tDiffuse.Sample(sSampler, bl) + tDiffuse.Sample(sSampler, bm) + tDiffuse.Sample(sSampler, br);
    float4 sum = top_sum + middle_sum + bottom_sum;
    float4 avg = sum / 9.0f;
    return avg;
}

float4 PixelFunction(ps_in_t input) : SV_Target0
{
    float4 albedo = tDiffuse.Sample(sSampler, input.uv);
    float4 diffuse = albedo * input.color;
    float4 fadeColor = g_fadeColor;
    float percent = g_fadePercent;
    float w = 0.0f;
    float h = 0.0f;
    tDiffuse.GetDimensions(w, h);
    switch(g_effectIndex) {
    case 0:
        return FadeIn(fadeColor, diffuse, percent);
    case 1:
        return FadeOut(fadeColor, diffuse, percent);
    case 2:
        return Lumosity(diffuse);
    case 3:
        return Sepia(diffuse);
    case 4:
        return CircularGradient(input.uv, diffuse, g_gradRadius, g_gradColor);
    case 5:
        return SquareBlur(input.uv, w, h, diffuse);
    default:
        return diffuse;
    }
}
