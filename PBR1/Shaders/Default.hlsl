//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 4
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif

// Include structures and functions for lighting.
#include "LightingUtil.hlsl"

// Constant data that varies per frame.

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
	float  gMetallic;
};

// Constant data that varies per material.
cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};
 
struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    const float PI = 3.14159265359f;
    // pbr代码所在
    float3 N = normalize(pin.NormalW);
    float3 V = normalize(gEyePosW - pin.PosW);
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    float3 albedo = gDiffuseAlbedo;
    F0 = lerp(F0, albedo, gMetallic);

    float3 Lo = float3(0.f, 0.f, 0.f);

    for (int i=0; i<4; ++i)
    {
        float3 LightPos = gLights[i].Position;
        float3 L = normalize(LightPos - pin.PosW);
        float3 H = normalize(V + L);
        float dis = length(LightPos - pin.PosW);
        float attenuation = 1.f / (dis * dis);
        float3 radiance = gLights[i].Strength * attenuation;

        float NDF = DistributionGGX(N, H, gRoughness);
        float G = GeometrySmith(N, V, L, gRoughness);
        float3 F = fresnelSchlick(saturate(dot(H, V)), F0); //SchlickFresnel(F0, N, V);

        float3 nominator = NDF * G * F;
        //float3 nominator = NDF * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0f);
        float3 specular = nominator / max(denominator, 0.001);

        float3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        float3 kD = float3(1.f, 1.f, 1.f) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD = kD * (1.f - gMetallic);

        // // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0f);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        //Lo += float3(gMetallic, gMetallic, gMetallic);
        //Lo += F;
        //Lo += (float3(NDF, NDF, NDF));// * radiance * NdotL;
        //Lo += float3(G, G, G);
    }

    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo;
    float3 color = ambient + Lo;
    color = color / (color + float3(1.f, 1.f, 1.f));
    // gamma correct
    float gammaFactor = 1.0f/2.2f;
    color = pow(color, float3(gammaFactor, gammaFactor, gammaFactor));

    float4 litColor = float4(color, 1.f);
    return litColor;
}


