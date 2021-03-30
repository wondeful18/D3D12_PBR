#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"

//----------------------------------------------------------------------
// 物体常量
//----------------------------------------------------------------------
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

//----------------------------------------------------------------------
// Pass常量
//----------------------------------------------------------------------
struct PassConstants
{
    DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
    DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
    float cbPerObjectPad1 = 0.0f;
    DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
    DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
    float NearZ = 0.0f;
    float FarZ = 0.0f;
    float TotalTime = 0.0f;
    float DeltaTime = 0.0f;

	// 环境光
    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// 光的数组，最大支持16盏灯
	// 索引 [0, NUM_DIR_LIGHTS) 是方向光
	// 索引 [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) 是点光
	// 索引 [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS) 是聚光灯
	Light Lights[MaxLights];
};

//----------------------------------------------------------------------
// 顶点数据结构
//----------------------------------------------------------------------
struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
};

//----------------------------------------------------------------------
// 帧资源，存有CPU为构建每帧命令列表所需的资源
//----------------------------------------------------------------------
struct FrameResource
{
public:
	// 构造析构函数
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

	// 在GPU处理完与此命令分配器相关的命令之前，我们不能对它进行重置。
	// 所以每一帧都要有它自己的命令分配器
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// 在GPU执行完引用此常量缓冲区的命令之前，我们不能对它进行更新。
	// 因此每一帧都要有它们自己的常量缓冲区

	// Pass常量缓冲区
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	// 材质常量缓冲区
    std::unique_ptr<UploadBuffer<MyMaterialConstants>> MaterialCB = nullptr;
	// 物体常量缓冲区
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	// 通过围栏值将命令标记到此围栏点，这使我们可以检测到GPU是否还在使用这些帧资源
    UINT64 Fence = 0;
};