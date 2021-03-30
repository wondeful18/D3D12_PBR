//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

//---------------------------------------------------------------------------------------------
// ������Ϣ������
//---------------------------------------------------------------------------------------------
LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	// ������Ҫ����hwnd����Ϊ���ǿ�����CreateWindows����֮ǰ�����յ���Ϣ��
	// ��ʱ��mhMainWnd��ʱû��¼����Ч���ھ��
    return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

//---------------------------------------------------------------------------------------------
// ����ʵ������ָ��
//---------------------------------------------------------------------------------------------
D3DApp* D3DApp::mApp = nullptr;


//---------------------------------------------------------------------------------------------
// ��ȡ����ʵ������
//---------------------------------------------------------------------------------------------
D3DApp* D3DApp::GetApp()
{
    return mApp;
}

//---------------------------------------------------------------------------------------------
// ���캯��
//---------------------------------------------------------------------------------------------
D3DApp::D3DApp(HINSTANCE hInstance)
:	mhAppInst(hInstance)
{
    // Only one D3DApp can be constructed.
    assert(mApp == nullptr);
	// ��¼����ʵ������Ϊ�Լ�
    mApp = this;
}

//---------------------------------------------------------------------------------------------
// ��������
//---------------------------------------------------------------------------------------------
D3DApp::~D3DApp()
{
	// ���D3D�豸��Ϊ��
	if(md3dDevice != nullptr)
		// ˢ���������
		FlushCommandQueue();
}

//---------------------------------------------------------------------------------------------
// ��ȡ���̾��
//---------------------------------------------------------------------------------------------
HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}

//---------------------------------------------------------------------------------------------
// ��ȡ�����ھ��
//---------------------------------------------------------------------------------------------
HWND D3DApp::MainWnd()const
{
	return mhMainWnd;
}

//---------------------------------------------------------------------------------------------
// ���ڳ����
//---------------------------------------------------------------------------------------------
float D3DApp::AspectRatio()const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

//---------------------------------------------------------------------------------------------
// ��ȡ������״̬
//---------------------------------------------------------------------------------------------
bool D3DApp::Get4xMsaaState()const
{
    return m4xMsaaState;
}

//---------------------------------------------------------------------------------------------
// ���÷�����״̬
//---------------------------------------------------------------------------------------------
void D3DApp::Set4xMsaaState(bool value)
{
	// ������ϴε�״̬��һ��
    if(m4xMsaaState != value)
    {
		// ��¼���·�����״̬
        m4xMsaaState = value;

        // Recreate the swapchain and buffers with new multisample settings.

		// ���´���������
        CreateSwapChain();
		// ��Ӧ���ڸı��С
        OnResize();
    }
}

//---------------------------------------------------------------------------------------------
// ����
//---------------------------------------------------------------------------------------------
int D3DApp::Run()
{
	MSG msg = {0};
 
	// ���ü�ʱ��
	mTimer.Reset();

	// ֻҪ��Ϣ�����˳�����
	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		// �����Windows��Ϣ��������
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		// ����ִ�ж���/��Ϸ����
		else
        {	
			// ��ʱ������һ֡
			mTimer.Tick();

			// ֻҪ������ͣ
			if( !mAppPaused )
			{
				// ����֡��
				CalculateFrameStats();
				// ��Ӧ����
				Update(mTimer);
				// ��Ӧ����
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
// ��ʼ��
//---------------------------------------------------------------------------------------------
bool D3DApp::Initialize()
{

	// ��ʼ������
	if (!InitMainWindow())
		return false;

	// ��ʼ��D3D
	if (!InitDirect3D())
		return false;

    // ִ�г�ʼ���ڸı��С���� Do the initial resize code.
    OnResize();

	return true;
}
 
//---------------------------------------------------------------------------------------------
// ������ȾĿ�������Լ����ģ��������
//---------------------------------------------------------------------------------------------
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	// ��ȾĿ�������
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	// �������������������壬����������
    rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	// �������ͣ���ȾĿ��
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	// �������
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	// ������ȾĿ��������
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// ���ģ��������
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	// ��������������һ��
    dsvHeapDesc.NumDescriptors = 1;
	// �������ͣ����ģ��
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	// �������ģ��������
    ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
        &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

//---------------------------------------------------------------------------------------------
// ��Ӧ���ڸı��С
//---------------------------------------------------------------------------------------------
void D3DApp::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
    assert(mDirectCmdListAlloc);

	// ˢ��������У������κ���Դ�޸�֮ǰ��Ҫˢ��
	FlushCommandQueue();

	// ����ֱ�������б������
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// �ͷ�֮ǰ����Դ�����ǽ�Ҫ�ؽ�����

	// �ͷŽ�������ǰ��̨������Դ
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();

	// �ͷ����ģ�建����Դ
    mDepthStencilBuffer.Reset();
	
	// �ı佻�����Ļ���ߴ�
    ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount, 
		mClientWidth, mClientHeight, 
		mBackBufferFormat, 
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	// ��ǰ�󱸻���������0
	mCurrBackBuffer = 0;
 
	// ��ȾĿ�������Ѿ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

	// ���ڽ�������ÿ������
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		// ��ȡ��������ǰ�������
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		// ������ȾĿ������������¼���������ָ���Ŀ����������
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		// ƫ�Ƶ��������ѵ���һ��������
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

    // �������/ģ�建����Դ�����ṹ��
    D3D12_RESOURCE_DESC depthStencilDesc;
	// ��Դά�ȣ�Ϊһ��2D����ά��
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mClientWidth;
    depthStencilDesc.Height = mClientHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
	// ���ģ���ʽ��DXGI_FORMAT_D24_UNORM_S8_UINT
    depthStencilDesc.Format = mDepthStencilFormat;
	// ���ز�����ÿ�����صĲ�������
    depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	// ���ز�������������
    depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	// ��ǣ�������Ⱥ�ģ��
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	// �����Դ���Ż�ֵ
    D3D12_CLEAR_VALUE optClear;
	// ����Ż��ĸ�ʽ
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

	// ������Ȼ�����Դ������Ĭ�϶�
    ThrowIfFailed(md3dDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	// ��Դ��Ŷѵ����ԣ�Ĭ�϶ѣ�ΨGPU���ʣ�
		D3D12_HEAP_FLAG_NONE,	// �Ѷ�����
        &depthStencilDesc,	// �������/ģ�建����Դ�����ṹ��
		D3D12_RESOURCE_STATE_COMMON,
        &optClear,	// �����Դ���Ż��ṹ
        IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

    // �������ģ�����������ô���Դ�ĸ�ʽ��Ϊ������Դ�ĵ�0 mip�㴴���������������뵽���ģ��������
    md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

    // ����Դ�ӳ�ʼ״̬ת��Ϊ��Ϊ��ȿ�д��״̬
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// ִ�иı䴰�ڴ�С����
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// �ȴ�ֱ���ı䴰�ڴ�С���
	FlushCommandQueue();

	// �����ӿڽṹ���Ա㸲�Ǵ��ڴ�С
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width    = static_cast<float>(mClientWidth);
	mScreenViewport.Height   = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	// ��¼�µĲü�����
    mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}
 
//---------------------------------------------------------------------------------------------
// ��Ϣ������
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
// ��ʼ��������
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
// ��ʼ��D3D
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

	// ����DX�����ӿ�
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// ���Դ���Ӳ���豸
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

	// ����Χ��
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	// ��ȡ��ȾĿ�������ߴ�
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	// ��ȡ���ģ�������ߴ�
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	// ��ȡ ����/��ɫ����Դ/������������ߴ�
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Check 4X MSAA quality support for our back buffer format.
    // All Direct3D 11 capable devices support 4X MSAA for all render 
    // target formats, so we only need to check quality support.

	// ������ǵĺ󱸻����ʽ��֧�ֵ�4X MSAA����
	// ���� Direct3D 11 ���ݵ��豸��֧���κ���ȾĿ���ʽ��4X MSAA
	// ��������֧�ּ��֧�ֵ�����

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	// ���Feature֧��
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");
	
#ifdef _DEBUG
	// ��������log
    LogAdapters();
#endif

	// ����������ض���
	CreateCommandObjects();
	// ����������
    CreateSwapChain();
	// ������ȾĿ�������Լ����ģ��������
    CreateRtvAndDsvDescriptorHeaps();

	return true;
}

//---------------------------------------------------------------------------------------------
// ����������ض���
//---------------------------------------------------------------------------------------------
void D3DApp::CreateCommandObjects()
{
	// �����������
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	// �������������ͣ�ֱ��
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	// �����������
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// ����ֱ�������б������
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	// ���������б�
	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // ������������� Associated command allocator
		nullptr,                   // ��ʼ����ˮ��״̬���� Initial PipelineStateObject
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// Start off in a closed state.  This is because the first time we refer 
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	// ����Ҫ�������б����ڹر�״̬����Ϊ��һ�����������б�ʱ�����ǻ���������
	// ���ڵ�������֮ǰ��Ҫ����ر�
	mCommandList->Close();
}

//---------------------------------------------------------------------------------------------
// ����������
//---------------------------------------------------------------------------------------------
void D3DApp::CreateSwapChain()
{
    // Release the previous swapchain we will be recreating.
	// �ͷ��ϴν�������COM��������ComPtr�����ú���
    mSwapChain.Reset();

	// ������������
    DXGI_SWAP_CHAIN_DESC sd;
	// ���ڿ��
    sd.BufferDesc.Width = mClientWidth;
    sd.BufferDesc.Height = mClientHeight;
	// ˢ����
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
	// �󱸻����ʽ
    sd.BufferDesc.Format = mBackBufferFormat;
	// ɨ���ߴ���
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	// ���ز�������
    sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	// ���ز�������
    sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	// ������Ҫ����������󱸻����Ա����
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// �������
    sd.BufferCount = SwapChainBufferCount;
	// �������
    sd.OutputWindow = mhMainWnd;
    sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Note: Swap chain uses queue to perform flush.
	// ע�⣬��������Ҫͨ�������б�������ˢ��
    ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd, 
		mSwapChain.GetAddressOf()));
}

//---------------------------------------------------------------------------------------------
// ˢ���������
//---------------------------------------------------------------------------------------------
void D3DApp::FlushCommandQueue()
{
	// ����Χ��ֵ������������Χ���� Advance the fence value to mark commands up to this fence point.
    mCurrentFence++;

    // Add an instruction to the command queue to set a new fence point.  Because we 
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	// ���һ��ָ����������������һ���µ�Χ���㡣��Ϊ��������GPUʱ���ߣ�����µ�Χ������
	// GPU��ɴ������б����Signal�����ȼ����ߵ�����֮ǰ���������á�
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	// �ȴ���ֱ��GPU������Χ���������
    if(mFence->GetCompletedValue() < mCurrentFence)
	{
		// ����һ���¼�
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        // Fire event when GPU hits current fence.
		// ��GPUִ�е���ǰΧ��ʱ����һ���¼�
        ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

        // Wait until the GPU hits current fence event is fired.
		// �ȴ�ֱ��GPU�����¼�
		WaitForSingleObject(eventHandle, INFINITE);
		// �ر�����¼����
        CloseHandle(eventHandle);
	}
}

//---------------------------------------------------------------------------------------------
// ��ȡ��ǰ�󱸻���
//---------------------------------------------------------------------------------------------
ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

//---------------------------------------------------------------------------------------------
// ��ǰ�󱸻�������
//---------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

//---------------------------------------------------------------------------------------------
// ���ģ������
//---------------------------------------------------------------------------------------------
D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

//---------------------------------------------------------------------------------------------
// ����֡��
//---------------------------------------------------------------------------------------------
void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	// �������ÿ��ƽ��֡�ʣ���������Ⱦһ֡��ƽ��ʱ������Щ��Ϣ��������Windows������
    
	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	// ����1��ƽ��֡��
	if( (mTimer.TotalTime() - timeElapsed) >= 1.0f )
	{
		// �����֡��
		float fps = (float)frameCnt; // fps = frameCnt / 1
		// 1֡ƽ��ʱ��
		float mspf = 1000.0f / fps;

        wstring fpsStr = to_wstring(fps);
        wstring mspfStr = to_wstring(mspf);

        wstring windowText = mMainWndCaption +
            L"    fps: " + fpsStr +
            L"   mspf: " + mspfStr;

		// ���õ����ڱ����ı�
        SetWindowText(mhMainWnd, windowText.c_str());
		
		// ���ã��Ա��������
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

//---------------------------------------------------------------------------------------------
// ��������log
//---------------------------------------------------------------------------------------------
void D3DApp::LogAdapters()
{
    UINT i = 0;
    IDXGIAdapter* adapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;

	// ��������������
    while(mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
		// ��ȡ����������
        DXGI_ADAPTER_DESC desc;
        adapter->GetDesc(&desc);

        std::wstring text = L"***Adapter: ";
        text += desc.Description;
        text += L"\n";

		// �����������������������ı�
        OutputDebugString(text.c_str());

        adapterList.push_back(adapter);
        
        ++i;
    }

	// ���ÿ�������������
    for(size_t i = 0; i < adapterList.size(); ++i)
    {
        LogAdapterOutputs(adapterList[i]);
        ReleaseCom(adapterList[i]);
    }
}

//---------------------------------------------------------------------------------------------
// �������������log
//---------------------------------------------------------------------------------------------
void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
    UINT i = 0;
    IDXGIOutput* output = nullptr;

	// ����������������������
    while(adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
    {
		// ��ȡ�������
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        
        std::wstring text = L"***Output: ";
        text += desc.DeviceName;
        text += L"\n";
        OutputDebugString(text.c_str());

		// ��ָ���������������ʾģʽ��log
        LogOutputDisplayModes(output, mBackBufferFormat);

        ReleaseCom(output);

        ++i;
    }
}

//---------------------------------------------------------------------------------------------
// ��ָ���������������ʾģʽ��log
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