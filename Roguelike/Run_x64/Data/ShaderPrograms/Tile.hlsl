
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

struct vs_in_t {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct gs_in_t {
    float2 position : WORLD;
    float4 color : COLOR;
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


gs_in_t VertexFunction(vs_in_t input_vertex) {
    gs_in_t output;

    float4 local = float4(input_vertex.position, 1.0f);
    float4 world = mul(local, g_MODEL);

    output.position = world.xy;
    output.color = input_vertex.color;

    return output;
}

[maxvertexcount(4)]
void GeometryFunction(point gs_in_t input[1], inout TriangleStream<ps_in_t> stream) {
    ps_in_t output = (ps_in_t)0;

    float px = input[0].position.x;
    float py = input[0].position.y;

    //TODO: Have fun with the world positions here...

    //float t = g_GAME_TIME;
    //float A = 1.0f;
    //float f = 1.0f;
    //float w = 0.0f;

    //px += 0.5f * cos(t);
    //py += 0.5f * sin(t);

    float2 new_position = float2(px, py);
    float4 world = float4(new_position, 0.0f, 1.0f);

    float2 tl_offset = float2(0.0f, -1.0f);
    float2 bl_offset = float2(0.0f, 0.0f);
    float2 tr_offset = float2(1.0f, -1.0f);
    float2 br_offset = float2(1.0f, 0.0f);

    float2 mapDims = float2(0.0f, 0.0f);
    tNormal.GetDimensions(mapDims.x, mapDims.y);
    float2 mapUVs = float2(1.0f / mapDims.x, 1.0f / mapDims.y);

    float4 index_color = tNormal.SampleLevel(sSampler, world.xy * mapUVs, 0);
    float2 index_ids = index_color.rg * 255.0f;
    float2 texCoords = 1.0f / 65.0f;
    float2 uv_mins = index_ids * texCoords;
    float2 uv_maxs = (index_ids + 1.0f) * texCoords;
    //float2 tileAtlasDims = float2(1.0f, 1.0f);
    //tDiffuse.GetDimensions(tileAtlasDims.x, tileAtlasDims.y);
    //float tileAtlasArea = tileAtlasDims.x * tileAtlasDims.y;
    //const float epsilon = 1.0f / tileAtlasArea;
    //uv_mins += epsilon;
    //uv_maxs -= epsilon;

    //Bottom Left
    float2 offset = bl_offset;
    float4 view = mul(float4(world.xy + offset, 0.0f, 1.0f), g_VIEW);
    float4 clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input[0].color;
    output.uv = float2(uv_mins.x, uv_maxs.y);
    stream.Append(output);

    //Top Left
    offset = tl_offset;
    view = mul(float4(world.xy + offset, 0.0f, 1.0f), g_VIEW);
    clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input[0].color;
    output.uv = float2(uv_mins.x, uv_mins.y);
    stream.Append(output);

    //Bottom Right
    offset = br_offset;
    view = mul(float4(world.xy + offset, 0.0f, 1.0f), g_VIEW);
    clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input[0].color;
    output.uv = float2(uv_maxs.x, uv_maxs.y);
    stream.Append(output);

    //Top Right
    offset = tr_offset;
    view = mul(float4(world.xy + offset, 0.0f, 1.0f), g_VIEW);
    clip = mul(view, g_PROJECTION);

    output.position = clip;
    output.color = input[0].color;
    output.uv = float2(uv_maxs.x, uv_mins.y);
    stream.Append(output);

    stream.RestartStrip();
}

float4 PixelFunction(ps_in_t input_pixel) : SV_Target0{
    float4 albedo = tDiffuse.Sample(sSampler, input_pixel.uv);
    float4 final_color = albedo * input_pixel.color;
    clip(final_color.a - 0.1);
    return final_color;
}
