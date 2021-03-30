#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"

//----------------------------------------------------------------------
// ���峣��
//----------------------------------------------------------------------
struct ObjectConstants
{
    DirectX::XMFLOAT4X4 World = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
};

//----------------------------------------------------------------------
// Pass����
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

	// ������
    DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// ������飬���֧��16յ��
	// ���� [0, NUM_DIR_LIGHTS) �Ƿ����
	// ���� [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) �ǵ��
	// ���� [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS) �Ǿ۹��
	Light Lights[MaxLights];
};

//----------------------------------------------------------------------
// �������ݽṹ
//----------------------------------------------------------------------
struct Vertex
{
    DirectX::XMFLOAT3 Pos;
    DirectX::XMFLOAT3 Normal;
};

//----------------------------------------------------------------------
// ֡��Դ������CPUΪ����ÿ֡�����б��������Դ
//----------------------------------------------------------------------
struct FrameResource
{
public:
	// ������������
    FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount);
    FrameResource(const FrameResource& rhs) = delete;
    FrameResource& operator=(const FrameResource& rhs) = delete;
    ~FrameResource();

	// ��GPU��������������������ص�����֮ǰ�����ǲ��ܶ����������á�
	// ����ÿһ֡��Ҫ�����Լ������������
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// ��GPUִ�������ô˳���������������֮ǰ�����ǲ��ܶ������и��¡�
	// ���ÿһ֡��Ҫ�������Լ��ĳ���������

	// Pass����������
    std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	// ���ʳ���������
    std::unique_ptr<UploadBuffer<MyMaterialConstants>> MaterialCB = nullptr;
	// ���峣��������
    std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	// ͨ��Χ��ֵ�������ǵ���Χ���㣬��ʹ���ǿ��Լ�⵽GPU�Ƿ���ʹ����Щ֡��Դ
    UINT64 Fence = 0;
};