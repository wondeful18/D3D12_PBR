//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 16

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd-d) / (falloffEnd - falloffStart));
}

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0)*(f0*f0*f0*f0*f0);

    return reflectPercent;
}

// ----------------------------------------------------------------------------
// 菲涅尔方程 当光线碰撞到一个表面的时候，菲涅尔方程会根据观察角度告诉我们被反射的光线所占的百分比
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    float OneSubCos = 1.f - cosTheta;

    return F0 + (1.f - F0) * OneSubCos * OneSubCos * OneSubCos * OneSubCos * OneSubCos;
}

// ----------------------------------------------------------------------------
// 正态分布函数：估算在受到表面粗糙度的影响下，取向方向与中间向量一致的微平面的数量。 DFG的D
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = 3.14159265359f * denom * denom;

    return nom / max(denom, 0.0000001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

// ----------------------------------------------------------------------------
// 几何函数 GGX与Schlick-Beckmann近似的结合体
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// ----------------------------------------------------------------------------
// 几何函数 史密斯法(Smith’s method)  将观察方向（几何遮蔽(Geometry Obstruction)）
// 和光线方向向量（几何阴影(Geometry Shadowing)）都考虑进去
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    // 观察方向
    float NdotV = max(dot(N, V), 0.0);
    // 光线方向
    float NdotL = max(dot(N, L), 0.0);
    // 观察方向的几何遮蔽
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    // 光线方向的几何阴影
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

