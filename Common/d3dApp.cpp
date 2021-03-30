//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

//---------------------------------------------------------------------------------------------
// 窗口消息处理函数
//---------------------------------------------------------------------------------------------
LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	// 这里需要传入hwnd是因为我们可以在CreateWindows返回之前，就收到消息，
	// 此时的mhMainWnd暂时没记录到有效窗口句柄
    return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

//---------------------------------------------------------------------------------------------
// 单件实例对象指针
//---------------------------------------------------------------------------------------------
D3DApp* D3DApp::mApp = nullptr;


//---------------------------------------------------------------------------------------------
// 获取单件实例对象
//---------------------------------------------------------------------------------------------
D3DApp* D3DApp::GetApp()
{
    return mApp;
}

//---------------------------------------------------------------------------------------------
// 构造函数
//---------------------------------------------------------------------------------------------
D3DApp::D3DApp(HINSTANCE hInstance)
:	mhAppInst(hInstance)
{
    // Only one D3DApp can be constructed.
    assert(mApp == nullptr);
	// 记录单件实例对象为自己
    mApp = this;
}

//---------------------------------------------------------------------------------------------
// 析构函数
//---------------------------------------------------------------------------------------------
D3DApp::~D3DApp()
{
	// 如果D3D设备不为空
	if(md3dDevice != nullptr)
		// 刷新命令队列
		FlushCommandQueue();
}

//---------------------------------------------------------------------------------------------
// 获取进程句柄
//---------------------------------------------------------------------------------------------
HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}

//---------------------------------------------------------------------------------------------
// 获取主窗口句柄
//---------------------------------------------------------------------------------------------
HWND D3DApp::MainWnd()const
{
	return mhMainWnd;
}

//---------------------------------------------------------------------------------------------
// 窗口长宽比
//---------------------------------------------------------------------------------------------
float D3DApp::AspectRatio()const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

//---------------------------------------------------------------------------------------------
// 获取反走样状态
//---------------------------------------------------------------------------------------------
bool D3DApp::Get4xMsaaState()const
{
    return m4xMsaaState;
}

//---------------------------------------------------------------------------------------------
// 设置反走样状态
//---------------------------------------------------------------------------------------------
void D3DApp::Set4xMsaaState(bool value)
{
	// 如果和上次的状态不一样
    if(m4xMsaaState != value)
    {
		// 记录最新反走样状态
        m4xMsaaState = value;

        // Recreate the swapchain and buffers with new multisample settings.

		// 重新创建交换链
        CreateSwapChain();
		// 响应窗口改变大小
        OnResize();
    }
}

//---------------------------------------------------------------------------------------------
// 运行
//---------------------------------------------------------------------------------------------
int D3DApp::Run()
{
	MSG msg = {0};
 
	// 重置计时器
	mTimer.Reset();

	// 只要消息不是退出程序
	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		// 如果有Windows消息则处理它们
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		// 否则执行动画/游戏代码
		else
        {	
			// 计时器运行一帧
			mTimer.Tick();

			// 只要程序不暂停
			if( !mAppPaused )
			{
				// 计算帧率
				CalculateFrameStats();
				// 响应更新
				Update(mTimer);
				// 响应绘制
                Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
        }
    }

	return (int)msg.wParam;
}

//---------------------------------------------------------------------------------------------
// 初始化
//---------------------------------------------------------------------------------------------
bool D3DApp::Initialize()
{

	// 初始化窗口
	if (!InitMainWindow())
		return false;

	// 初始化D3D
	if (!InitDirect3D())
		return false;

    // 执行初始窗口改变大小代码 Do the initial resize code.
    OnResize();

	return true;
}
 
//---------------------------------------------------------------------------------------------
// 创建渲染目标描述以及深度模板描述堆
//---------------------------------------------------------------------------------------------
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	// 渲染目标堆描述
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	// 描述个数，有两个缓冲，则两个描述
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	// 描述类型：渲染目标
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// 描述标记
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	// 创建渲染目标描述堆
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// 深度模板描述堆
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	// 描述个数：仅有一个
    dsvHeapDesc.NumDescriptors = 1;
	// 描述类型：深度模板
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	// 创建深度模板描述堆
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

//---------------------------------------------------------------------------------------------
// 响应窗口改变大小
//---------------------------------------------------------------------------------------------
void D3DApp::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
    assert(mDirectCmdListAlloc);

	// 刷新命令队列，当有任何资源修改之前都要刷新
	FlushCommandQueue();

	// 重置直接命令列表分配器
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 释放之前的资源，我们将要重建它们

	// 释放交换链的前后台缓冲资源
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();

	// 释放深度模板缓冲资源
    mDepthStencilBuffer.Reset();
	
	// 改变交换链的缓冲尺寸
    ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		mClientWidth, mClientHeight, 
		mBackBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	// 当前后备缓冲索引：0
	mCurrBackBuffer = 0;
 
	// 渲染目标描述堆句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

	// 对于交换链的每个缓冲
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		// 获取交换链当前缓冲对象
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		// 创建渲染目标描述，并记录到描述句柄指向的目标描述堆中
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		// 偏移到描述符堆的下一个缓冲区
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

    // 创建深度/模板缓冲资源描述结构体
    D3D12_RESOURCE_DESC depthStencilDesc;
	// 资源维度，为一个2D纹理维度
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
	// 深度模板格式：DXGI_FORMAT_D24_UNORM_S8_UINT
    depthStencilDesc.Format = mDepthStencilFormat;
	// 多重采样的每个像素的采样次数
    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	// 多重采样的质量级别
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	// 标记：允许深度和模板
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// 清除资源的优化值
    D3D12_CLEAR_VALUE optClear;
	// 清除优化的格式
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

	// 创建深度缓冲资源，放在默认堆
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	// 资源存放堆的属性，默认堆（唯GPU访问）
		D3D12_HEAP_FLAG_NONE,	// 堆额外标记
        &depthStencilDesc,	// 创建深度/模板缓冲资源描述结构体
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,	// 清除资源的优化结构
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    // 创建深度模板描述：利用此资源的格式，为整个资源的第0 mip层创建描述符，并放入到深度模板描述堆
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

    // 将资源从初始状态转换为作为深度可写的状态
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 执行改变窗口大小命令
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 等待直到改变窗口大小完成
	FlushCommandQueue();

	// 更新视口结构，以便覆盖窗口大小
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mClientWidth);
	mScreenViewport.Height   = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	// 记录新的裁剪矩形
    mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}
 
//---------------------------------------------------------------------------------------------
// 消息处理函数
//---------------------------------------------------------------------------------------------
LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth  = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if( md3dDevice )
		{
			if( wParam == SIZE_MINIMIZED )
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				
				// Restoring from minimized state?
				if( mMinimized )
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if( mMaximized )
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if( mResizing )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing  = true;
		mTimer.Stop();
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing  = false;
		mTimer.Start();
		OnResize();
		return 0;
 
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
    case WM_KEYUP:
        if(wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        else if((int)wParam == VK_F2)
            Set4xMsaaState(!m4xMsaaState);

        return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//---------------------------------------------------------------------------------------------
// 初始化主窗口
//---------------------------------------------------------------------------------------------
bool D3DApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = mhAppInst;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = L"MainWnd";

	if( !RegisterClass(&wc) )
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mClientWidth, mClientHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(), 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0); 
	if( !mhMainWnd )
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

//---------------------------------------------------------------------------------------------
// 初始化D3D
//---------------------------------------------------------------------------------------------
bool D3DApp::InitDirect3D()
{
#if defined(DEBUG) || defined(_DEBUG) 
	// Enable the D3D12 debug layer.
{
	ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	debugController->EnableDebugLayer();
}
#endif

	// 创建DX工厂接口
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// 尝试创建硬件设备
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // default adapter
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&md3dDevice));

	// Fallback to WARP device.
	if(FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	// 创建围栏
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	// 获取渲染目标描述尺寸
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// 获取深度模板描述尺寸
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	// 获取 常量/着色器资源/无序访问描述尺寸
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Check 4X MSAA quality support for our back buffer format.
    // All Direct3D 11 capable devices support 4X MSAA for all render 
    // target formats, so we only need to check quality support.

	// 检测我们的后备缓冲格式所支持的4X MSAA质量
	// 所有 Direct3D 11 兼容的设备都支持任何渲染目标格式的4X MSAA
	// 所以我们支持检测支持的质量

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	// 检测Feature支持
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
	
#ifdef _DEBUG
	// 打设配器log
    LogAdapters();
#endif

	// 创建命令相关对象
	CreateCommandObjects();
	// 创建交换链
    CreateSwapChain();
	// 创建渲染目标描述以及深度模板描述堆
    CreateRtvAndDsvDescriptorHeaps();

	return true;
}

//---------------------------------------------------------------------------------------------
// 创建命令相关对象
//---------------------------------------------------------------------------------------------
void D3DApp::CreateCommandObjects()
{
	// 命令队列描述
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	// 队列描述的类型：直接
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// 创建命令队列
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// 创建直接命令列表分配器
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	// 创建命令列表
	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // 关联命令分配器 Associated command allocator
		nullptr,                   // 初始化流水线状态对象 Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	// 首先要将命令列表置于关闭状态，因为第一次引用命令列表时，我们会重置它，
	// 而在调用重置之前需要将其关闭
	mCommandList->Close();
}

//---------------------------------------------------------------------------------------------
// 创建交换链
//---------------------------------------------------------------------------------------------
void D3DApp::CreateSwapChain()
{
    // Release the previous swapchain we will be recreating.
	// 释放上次交换链的COM对象，用了ComPtr的重置函数
    mSwapChain.Reset();

	// 交换链的描述
    DXGI_SWAP_CHAIN_DESC sd;
	// 窗口宽高
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
	// 刷新率
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
	// 后备缓冲格式
    sd.BufferDesc.Format = mBackBufferFormat;
	// 扫描线次序
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	// 多重采样次数
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	// 多重采样质量
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	// 我们需要把它输出到后备缓冲以便输出
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// 缓冲个数
    sd.BufferCount = SwapChainBufferCount;
	// 输出窗口
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	// 注意，交换链需要通过命令列表对其进行刷新
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd, 
		mSwapChain.GetAddressOf()));
}

//---------------------------------------------------------------------------------------------
// 刷新命令队列
//---------------------------------------------------------------------------------------------
void D3DApp::FlushCommandQueue()
{
	// 递增围栏值，来标记命令到本围栏点 Advance the fence value to mark commands up to this fence point.
    mCurrentFence++;

    // Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	// 添加一个指令给命令队列来设置一个新的围栏点。因为我们是在GPU时间线，这个新的围栏点在
	// GPU完成处理所有比这个Signal的优先级更高的任务之前将不会设置。
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	// 等待，直到GPU完成这个围栏点的命令
    if(mFence->GetCompletedValue() < mCurrentFence)
	{
		// 创建一个事件
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.
		// 当GPU执行到当前围栏时触发一个事件
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

        // Wait until the GPU hits current fence event is fired.
		// 等待直到GPU触发事件
		WaitForSingleObject(eventHandle, INFINITE);
		// 关闭这个事件句柄
        CloseHandle(eventHandle);
	}
}

//---------------------------------------------------------------------------------------------
// 获取当前后备缓冲
//---------------------------------------------------------------------------------------------
ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

//---------------------------------------------------------------------------------------------
// 当前后备缓冲描述
//---------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

//---------------------------------------------------------------------------------------------
// 深度模板描述
//---------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

//---------------------------------------------------------------------------------------------
// 计算帧率
//---------------------------------------------------------------------------------------------
void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	// 代码计算每秒平均帧率，并计算渲染一帧的平均时长。这些信息被附加在Windows标题栏
    
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	// 计算1秒平均帧率
	if( (mTimer.TotalTime() - timeElapsed) >= 1.0f )
	{
		// 这秒的帧数
		float fps = (float)frameCnt; // fps = frameCnt / 1
		// 1帧平均时长
		float mspf = 1000.0f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);

        wstring windowText = mMainWndCaption +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

		// 设置到窗口标题文本
        SetWindowText(mhMainWnd, windowText.c_str());
		
		// 重置，以便下秒计算
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

//---------------------------------------------------------------------------------------------
// 打设配器log
//---------------------------------------------------------------------------------------------
void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;

	// 遍历所有适配器
    while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
		// 获取适配器描述
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

		// 输出这个适配器描述到调试文本
        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        
        ++i;
    }

	// 输出每个适配器的输出
    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

//---------------------------------------------------------------------------------------------
// 打适配器输出的log
//---------------------------------------------------------------------------------------------
void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;

	// 遍历这个适配器的所有输出
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
		// 获取输出描述
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

		// 打指定适配器输出的显示模式的log
        LogOutputDisplayModes(output, mBackBufferFormat);

        ReleaseCom(output);

        ++i;
    }
}

//---------------------------------------------------------------------------------------------
// 打指定适配器输出的显示模式的log
//---------------------------------------------------------------------------------------------
void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
    UINT count = 0;
    UINT flags = 0;

    // Call with nullptr to get list count.
    output->GetDisplayModeList(format, flags, &count, nullptr);

    std::vector<DXGI_MODE_DESC> modeList(count);
    output->GetDisplayModeList(format, flags, &count, &modeList[0]);

    for(auto& x : modeList)
    {
        UINT n = x.RefreshRate.Numerator;
        UINT d = x.RefreshRate.Denominator;
        std::wstring text =
            L"Width = " + std::to_wstring(x.Width) + L" " +
            L"Height = " + std::to_wstring(x.Height) + L" " +
            L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
            L"\n";

        ::OutputDebugString(text.c_str());
    }
}