
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
    int2 g_resolution;
    // Hardness of scanline.
    //  -8.0 = soft
    // -16.0 = medium
    float hardScan;
    // Hardness of pixels in scanline.
    // -2.0 = soft
    // -4.0 = hard
    float hardPix;
    // Amount of shadow mask.
    float maskDark;
    float maskLight;

    // Display warp.
    // 0.0 = none
    // 1.0/8.0 = extreme
    float2 warp;
    float2 res;
    float2 padding;
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

//----------------------------------------------------------------------------------
//
// SHADERTOY SCANLINES
//
//----------------------------------------------------------------------------------

// PUBLIC DOMAIN CRT STYLED SCAN-LINE SHADER
//
//   by Timothy Lottes
//
// This is more along the style of a really good CGA arcade monitor.
// With RGB inputs instead of NTSC.
// The shadow mask example has the mask rotated 90 degrees for less chromatic aberration.
//
// Left it unoptimized to show the theory behind the algorithm.
//
// It is an example what I personally would want as a display option for pixel art games.
// Please take and use, change, or whatever.
//
// 07/15/2019 -- cugone: Converted to HLSL

//------------------------------------------------------------------------

float3 Fetch(float2 pos, float2 off);
float ToLinear1(float c);
float3 ToLinear(float3 c);
float ToSrgb1(float c);
float3 ToSrgb(float3 c);

// sRGB to Linear.
// Assuing using sRGB typed textures this should not be needed.
float ToLinear1(float c) { return(c <= 0.04045f) ? c / 12.92f : pow((c + 0.055f) / 1.055f, 2.4f); }
float3 ToLinear(float3 c) { return float3(ToLinear1(c.r), ToLinear1(c.g), ToLinear1(c.b)); }

// Linear to sRGB.
// Assuming using sRGB typed textures this should not be needed.
float ToSrgb1(float c) { return(c < 0.0031308f ? c * 12.92f : 1.055f*pow(c, 0.41666f) - 0.055f); }
float3 ToSrgb(float3 c) { return float3(ToSrgb1(c.r), ToSrgb1(c.g), ToSrgb1(c.b)); }

// Nearest emulated sample given floating point position and texel offset.
// Also zero's off screen.
float3 Fetch(float2 pos, float2 off) {
    pos = floor(pos * res + off) / res;
    float2 pos_center = pos - float2(0.5f, 0.5f);
    float abs_x = abs(pos_center.x);
    float abs_y = abs(pos_center.y);
    float larger_coord = max(abs_x, abs_y);
    float3 black = float3(0.0f, 0.0f, 0.0f);
    float3 albedo = tDiffuse.SampleBias(sSampler, pos.xy, -16.0f).rgb;
    float3 color = albedo;
    if(larger_coord > 0.5f) {
        color = black;
    }
    return ToLinear(color);
}

// Distance in emulated pixels to nearest texel.
float2 Dist(float2 pos) { pos = pos * res; return -((pos - floor(pos)) - float2(0.5f, 0.5f)); }

// 1D Gaussian.
float Gaus(float pos, float scale) { return exp2(scale*pos*pos); }

// 3-tap Gaussian filter along horz line.
float3 Horz3(float2 pos, float off) {
    float3 b = Fetch(pos, float2(-1.0f, off));
    float3 c = Fetch(pos, float2(0.0f, off));
    float3 d = Fetch(pos, float2(1.0f, off));
    float dst = Dist(pos).x;
    // Convert distance to weight.
    float scale = hardPix;
    float wb = Gaus(dst - 1.0f, scale);
    float wc = Gaus(dst + 0.0f, scale);
    float wd = Gaus(dst + 1.0f, scale);
    // Return filtered sample.
    return (b*wb + c * wc + d * wd) / (wb + wc + wd);
}

// 5-tap Gaussian filter along horz line.
float3 Horz5(float2 pos, float off) {
    float3 a = Fetch(pos, float2(-2.0f, off));
    float3 b = Fetch(pos, float2(-1.0f, off));
    float3 c = Fetch(pos, float2(0.0f, off));
    float3 d = Fetch(pos, float2(1.0f, off));
    float3 e = Fetch(pos, float2(2.0f, off));
    float dst = Dist(pos).x;
    // Convert distance to weight.
    float scale = hardPix;
    float wa = Gaus(dst - 2.0f, scale);
    float wb = Gaus(dst - 1.0f, scale);
    float wc = Gaus(dst + 0.0f, scale);
    float wd = Gaus(dst + 1.0f, scale);
    float we = Gaus(dst + 2.0f, scale);
    // Return filtered sample.
    return (a*wa + b * wb + c * wc + d * wd + e * we) / (wa + wb + wc + wd + we);
}

// Return scanline weight.
float Scan(float2 pos, float off) {
    float dst = Dist(pos).y;
    return Gaus(dst + off, hardScan);
}

// Allow nearest three lines to effect pixel.
float3 Tri(float2 pos) {
    float3 a = Horz3(pos, -1.0);
    float3 b = Horz5(pos, 0.0);
    float3 c = Horz3(pos, 1.0);
    float wa = Scan(pos, -1.0);
    float wb = Scan(pos, 0.0);
    float wc = Scan(pos, 1.0);
    return a * wa + b * wb + c * wc;
}

// Distortion of scanlines, and end of screen alpha.
float2 Warp(float2 pos) {
    pos = pos * 2.0 - 1.0;
    pos *= float2(1.0 + (pos.y*pos.y)*warp.x, 1.0 + (pos.x*pos.x)*warp.y);
    return pos * 0.5 + 0.5;
}

// Shadow mask.
float3 Mask(float2 pos) {
    pos.x += pos.y*3.0;
    float3 mask = float3(maskDark, maskDark, maskDark);
    pos.x = frac(pos.x / 6.0);
    if(pos.x < 0.333)mask.r = maskLight;
    else if(pos.x < 0.666)mask.g = maskLight;
    else mask.b = maskLight;
    return mask;
}

// Draw dividing bars.
float Bar(float pos, float bar) { pos -= bar; return pos * pos < 4.0 ? 0.0 : 1.0; }

float4 Scanlines(float4 color, float2 uv) {
    float2 fragCoord = uv;
    float4 fragColor = color;
    float hardscan_copy = hardScan;
    float maskdark_copy = maskDark;
    float masklight_copy = maskLight;
    if(fragCoord.x < g_resolution.x*0.333f) {
        fragColor.rgb = Fetch(fragCoord.xy / g_resolution.xy + float2(0.333f, 0.0f), float2(0.0f, 0.0f));
    } else {
        float2 pos = Warp(fragCoord.xy / g_resolution.xy + float2(-0.333f, 0.0f));
        if(fragCoord.x < g_resolution.x*0.666f) {
            hardscan_copy = -12.0f;
            maskdark_copy = 1.0f;
            masklight_copy = 1.0f;
            pos = Warp(fragCoord.xy / g_resolution.xy);
        }
        fragColor.rgb = Tri(pos)*Mask(fragCoord.xy);
    }
    fragColor.a = color.a;
    fragColor.rgb *=
        Bar(fragCoord.x, g_resolution.x*0.333f)*
        Bar(fragCoord.x, g_resolution.x*0.666f);
    fragColor.rgb = ToSrgb(fragColor.rgb);
    return fragColor;
}

//----------------------------------------------------------------------------------
//
// SHADERTOY SCANLINES
//
//----------------------------------------------------------------------------------

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
        return Scanlines(diffuse, input.uv);
    case 3:
        return Lumosity(diffuse);
    case 4:
        return Sepia(diffuse);
    case 5:
        return CircularGradient(input.uv, diffuse, g_gradRadius, g_gradColor);
    default:
        return diffuse;
    }
}