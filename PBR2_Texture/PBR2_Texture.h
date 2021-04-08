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
// 帧资源个数：3个
//----------------------------------------------------------------------
const int gNumFrameResources = 3;

//----------------------------------------------------------------------
// 渲染项：存储绘制图形所需参数的轻量级结构体
//----------------------------------------------------------------------
struct RenderItem
{
	RenderItem() = default;

	// 描述物体局部空间相对世界空间的世界矩阵
	// 它定义了物体位于世界空间的位置，朝向与缩放
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// 弄脏标记：标记物体的相关数据已发生改变，意味着我们此时需要更新常量缓冲，
	// 由于每个FrameResource中都有一个物体常量缓冲区，所以我们需要对每个FrameResource都更新。
	// 即，当我们修改物体数据的时候，应当按NumFramesDirty = gNumFrameResources进行设置，
	// 从而使每个帧资源都得到更新
	int NumFramesDirty = gNumFrameResources;

	// 该索引指向的GPU常量缓冲区对应于当前渲染项的物体常量缓冲区
	UINT ObjCBIndex = -1;

	// 材质
	MyMaterial* Mat = nullptr;
	// 参与绘制的几何体
	MeshGeometry* Geo = nullptr;

	// 图元拓扑类型
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced 方法的参数
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};


//----------------------------------------------------------------------
// PBR纹理演示程序
//----------------------------------------------------------------------
class LitColumnsApp : public D3DApp
{
public:
	// 构造析构函数
	LitColumnsApp(HINSTANCE hInstance);
	LitColumnsApp(const LitColumnsApp& rhs) = delete;
	LitColumnsApp& operator=(const LitColumnsApp& rhs) = delete;
	~LitColumnsApp();

	// 初始化
	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update(const GameTimer& gt)override;
	// 绘制
	virtual void Draw(const GameTimer& gt)override;

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void AnimateMaterials(const GameTimer& gt);

	// 更新物体常量缓冲区
	void UpdateObjectCBs(const GameTimer& gt);
	// 更新材质常量缓冲区
	void UpdateMaterialCBs(const GameTimer& gt);
	// 更新主Pass常量缓冲区
	void UpdateMainPassCB(const GameTimer& gt);

	// 载入纹理
	void LoadTextures();
	// 构建根签名
	void BuildRootSignature();
	// 构建描述堆
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildShapeGeometry();
	void BuildPSOs();
	// 构建帧资源
	void BuildFrameResources();
	// 构建材质
	void BuildMaterials();
	// 构建渲染项
	void BuildRenderItems();
	// 绘制渲染项
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

	// 行列数
	int mRows = 7;
	int mColumns = 7;
};
