//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

//---------------------------------------------------------------------------------------------
// D3D的app基类
//---------------------------------------------------------------------------------------------
class D3DApp
{
protected:
	// 构造函数
    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;
	// 析构函数
    virtual ~D3DApp();

public:
	// 获取单件实例对象
    static D3DApp* GetApp();
    
	// 获取进程句柄
	HINSTANCE AppInst()const;
	// 获取主窗口句柄
	HWND      MainWnd()const;
	// 窗口长宽比
	float     AspectRatio()const;

	// 获取反走样状态
    bool Get4xMsaaState()const;
	// 设置反走样状态
    void Set4xMsaaState(bool value);

	// 运行
	int Run();
 
	// 初始化
    virtual bool Initialize();
	// 消息处理函数
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	// 创建渲染目标描述以及深度模板描述堆
    virtual void CreateRtvAndDsvDescriptorHeaps();
	// 响应窗口改变大小
	virtual void OnResize();
	// 响应更新
	virtual void Update(const GameTimer& gt)=0;
	// 响应绘制
	virtual void Draw(const GameTimer& gt)=0;

	// Convenience overrides for handling mouse input.
	
	// 响应鼠标按下
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	// 响应鼠标抬起
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	// 响应鼠标移动
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

protected:

	// 初始化主窗口
	bool InitMainWindow();
	// 初始化D3D
	bool InitDirect3D();
	// 创建命令相关对象
	void CreateCommandObjects();
	// 创建交换链
	void CreateSwapChain();

	// 刷新命令队列
	void FlushCommandQueue();

	// 获取当前后备缓冲
	ID3D12Resource* CurrentBackBuffer()const;
	// 当前后备缓冲描述
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	// 深度模板描述
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	// 计算帧率
	void CalculateFrameStats();

	// 打设配器log
	void LogAdapters();
	// 打指定适配器输出的log
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	// 打指定适配器输出的显示模式的log
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

    static D3DApp* mApp;			// 单件实例对象指针

    HINSTANCE mhAppInst = nullptr; // 进程句柄 application instance handle
    HWND      mhMainWnd = nullptr; // 主窗口句柄 main window handle
	bool      mAppPaused = false;  // app是否在暂停？ is the application paused?
	bool      mMinimized = false;  // app是否最小化了？is the application minimized?
	bool      mMaximized = false;  // app是否最大化了？is the application maximized?
	bool      mResizing = false;   // 是否正在改变窗口大小？are the resize bars being dragged?
    bool      mFullscreenState = false;// 是否全屏模式？fullscreen enabled

	// Set true to use 4X MSAA (?.1.8).  The default is false.
    bool      m4xMsaaState = false;    // 4X MSAA 是否有效？enabled
    UINT      m4xMsaaQuality = 0;      // 4X MSAA的质量等级

	// Used to keep track of the delta-time and game time (?.4).
	GameTimer mTimer;		// 用来记录帧间延时与游戏时间
	
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;		// DX工厂接口
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;		// 交换链
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;		// D3D设备

    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;				// 围栏
    UINT64 mCurrentFence = 0;								// 当前围栏值
	
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;			// 命令队列
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc; // 直接命令列表分配器
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;		// 命令列表

	static const int SwapChainBufferCount = 2;		// 交换链的缓冲个数，默认为2个
	int mCurrBackBuffer = 0;						// 当前后备缓冲索引
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];		// 交换链的缓冲数组
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;		// 深度模板缓冲

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;			// 渲染目标描述堆
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;			// 深度模板描述堆

    D3D12_VIEWPORT mScreenViewport; 		// 屏幕视口
    D3D12_RECT mScissorRect;				// 裁剪矩形

	UINT mRtvDescriptorSize = 0;			// 渲染目标描述尺寸
	UINT mDsvDescriptorSize = 0;			// 深度模板描述尺寸
	UINT mCbvSrvUavDescriptorSize = 0;		// 常量/着色器资源/无序访问描述尺寸

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring mMainWndCaption = L"d3d App";		// 主窗口标题
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;			// 设备类型：硬件设备
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;			// 后备缓冲格式：RGBA8888
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;	// 深度模板缓冲格式：深度24位，模板8位
	int mClientWidth = 800;			// 窗口宽
	int mClientHeight = 600;		// 窗口高
};

