#pragma once

#include "resource.h"

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "../Common/Camera.h"
#include "FrameResource.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

//----------------------------------------------------------------------
// ֡��Դ������3��
//----------------------------------------------------------------------
const int gNumFrameResources = 3;

//----------------------------------------------------------------------
// ��Ⱦ��洢����ͼ������������������ṹ��
//----------------------------------------------------------------------
struct RenderItem
{
	RenderItem() = default;

	// ��������ֲ��ռ��������ռ���������
	// ������������λ������ռ��λ�ã�����������
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Ū���ǣ�����������������ѷ����ı䣬��ζ�����Ǵ�ʱ��Ҫ���³������壬
	// ����ÿ��FrameResource�ж���һ�����峣��������������������Ҫ��ÿ��FrameResource�����¡�
	// �����������޸��������ݵ�ʱ��Ӧ����NumFramesDirty = gNumFrameResources�������ã�
	// �Ӷ�ʹÿ��֡��Դ���õ�����
	int NumFramesDirty = gNumFrameResources;

	// ������ָ���GPU������������Ӧ�ڵ�ǰ��Ⱦ������峣��������
	UINT ObjCBIndex = -1;

	// ����
	MyMaterial* Mat = nullptr;
	// ������Ƶļ�����
	MeshGeometry* Geo = nullptr;

	// ͼԪ��������
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced �����Ĳ���
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};


//----------------------------------------------------------------------
// PBR������ʾ����
//----------------------------------------------------------------------
class LitColumnsApp : public D3DApp
{
public:
	// ������������
	LitColumnsApp(HINSTANCE hInstance);
	LitColumnsApp(const LitColumnsApp& rhs) = delete;
	LitColumnsApp& operator=(const LitColumnsApp& rhs) = delete;
	~LitColumnsApp();

	// ��ʼ��
	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	// ����
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);

	// �������峣��������
	void UpdateObjectCBs(const GameTimer& gt);
	// ���²��ʳ���������
	void UpdateMaterialCBs(const GameTimer& gt);
	// ������Pass����������
	void UpdateMainPassCB(const GameTimer& gt);

	// ��������
	void LoadTextures();
	// ������ǩ��
	void BuildRootSignature();
	// ����������
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	// ����֡��Դ
	void BuildFrameResources();
	// ��������
	void BuildMaterials();
	// ������Ⱦ��
	void BuildRenderItems();
	// ������Ⱦ��
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);


	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
private:

	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<MyMaterial>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mOpaqueRitems;

	PassConstants mMainPassCB;

	Camera mCamera;

	POINT mLastMousePos;

	// ������
	int mRows = 7;
	int mColumns = 7;
};
