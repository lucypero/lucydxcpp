// hello
// here is what u gotta do:

// 3 directional lights
// 1 pointlight

// wvp
// world
// worldinvtranspose
// eyeposw

// mat

#include "light_helper.fx"

cbuffer cbPerFrame
{
    DirectionalLight gDirLights[3];
    PointLight gPointLight;

    float3 gEyePosW;

    float gFogStart;
    float gFogRange;
    float gFogColor;
};

cbuffer cbPerObject
{
    float4x4 gWorld;
    float4x4 gWorldInvTranspose;
    float4x4 gWorldViewProj;
    // float4x4 gTexTransform; // no textures yet
    Material gMaterial;
};


// todo: add texture stuff here

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;

    // Transform to world space.
    vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
    
    // Transform to homogeneous clip space.
    vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

    return vout;
}

float4 PS(VertexOut pin, uniform int gLightCount) : SV_Target
{
    // Interpolating normal can unnormalize it, so normalize it.
    pin.NormalW = normalize(pin.NormalW);

    // the toEye vector is used in lighting.
    float3 toEye = gEyePosW - pin.PosW;

    // Cache the distance to the eye from this surface point.
    float distToEye = length(toEye);

    // Normalize.
    toEye /= distToEye;

    //
    // Lighting.
    //

    // start with a sum of zero.
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // sum the light contribution from each light source.

    [unroll]
    for(int i = 0; i < gLightCount; ++i)
    {
        float4 A, D, S;
        ComputeToonDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye, A, D, S);

        ambient += A;
        diffuse += D;
        spec += S;
    }

    float4 A, D, S;
    ComputeToonPointLight(gMaterial, gPointLight, pin.PosW, pin.NormalW, toEye, A, D, S);

    ambient += A;
    diffuse += D;
    spec += S;

    float4 litColor = ambient + diffuse + spec;

    // Common to take alpha from diffuse material.
    litColor.a = gMaterial.Diffuse.a;

    return litColor;
}

technique11 Light1
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(1)));
    }
}

technique11 Light2
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(2)));
    }
}

technique11 Light3
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PS(3)));
    }
}