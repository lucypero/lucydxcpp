cbuffer cbPerObject
{
    float4x4 gWorldViewProj;
};

cbuffer timeUniform
{
    float gTime;
};

struct VertexIn
{
    float3 Pos: POSITION;
    float4 Color: COLOR;
};

struct VertexOut
{
    float4 PosH: SV_POSITION;
    float4 Color: COLOR;
};


VertexOut VS(VertexIn vin)
{

    vin.Pos.xy += 0.5f * sin(vin.Pos.x) * sin(3.0f * gTime);
    vin.Pos.z *= 0.6f + 0.4f * sin(2.0f*gTime);

    VertexOut vout;

    // Transform to homogeneous clip space.
    vout.PosH = mul(float4(vin.Pos, 1.0f), gWorldViewProj);

    // Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
//        return float4(1.0,0.0,0.0,1.0);
}

technique11 ColorTech
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetPixelShader(CompileShader(ps_5_0, PS()));
    }
}