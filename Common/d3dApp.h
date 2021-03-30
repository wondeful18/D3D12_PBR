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
// D3D��app����
//---------------------------------------------------------------------------------------------
class D3DApp
{
protected:
	// ���캯��
    D3DApp(HINSTANCE hInstance);
    D3DApp(const D3DApp& rhs) = delete;
    D3DApp& operator=(const D3DApp& rhs) = delete;
	// ��������
    virtual ~D3DApp();

public:
	// ��ȡ����ʵ������
    static D3DApp* GetApp();
    
	// ��ȡ���̾��
	HINSTANCE AppInst()const;
	// ��ȡ�����ھ��
	HWND      MainWnd()const;
	// ���ڳ����
	float     AspectRatio()const;

	// ��ȡ������״̬
    bool Get4xMsaaState()const;
	// ���÷�����״̬
    void Set4xMsaaState(bool value);

	// ����
	int Run();
 
	// ��ʼ��
    virtual bool Initialize();
	// ��Ϣ������
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:
	// ������ȾĿ�������Լ����ģ��������
    virtual void CreateRtvAndDsvDescriptorHeaps();
	// ��Ӧ���ڸı��С
	virtual void OnResize();
	// ��Ӧ����
	virtual void Update(const GameTimer& gt)=0;
	// ��Ӧ����
	virtual void Draw(const GameTimer& gt)=0;

	// Convenience overrides for handling mouse input.
	
	// ��Ӧ��갴��
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	// ��Ӧ���̧��
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	// ��Ӧ����ƶ�
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

protected:

	// ��ʼ��������
	bool InitMainWindow();
	// ��ʼ��D3D
	bool InitDirect3D();
	// ����������ض���
	void CreateCommandObjects();
	// ����������
	void CreateSwapChain();

	// ˢ���������
	void FlushCommandQueue();

	// ��ȡ��ǰ�󱸻���
	ID3D12Resource* CurrentBackBuffer()const;
	// ��ǰ�󱸻�������
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	// ���ģ������
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	// ����֡��
	void CalculateFrameStats();

	// ��������log
	void LogAdapters();
	// ��ָ�������������log
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	// ��ָ���������������ʾģʽ��log
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

    static D3DApp* mApp;			// ����ʵ������ָ��

    HINSTANCE mhAppInst = nullptr; // ���̾�� application instance handle
    HWND      mhMainWnd = nullptr; // �����ھ�� main window handle
	bool      mAppPaused = false;  // app�Ƿ�����ͣ�� is the application paused?
	bool      mMinimized = false;  // app�Ƿ���С���ˣ�is the application minimized?
	bool      mMaximized = false;  // app�Ƿ�����ˣ�is the application maximized?
	bool      mResizing = false;   // �Ƿ����ڸı䴰�ڴ�С��are the resize bars being dragged?
    bool      mFullscreenState = false;// �Ƿ�ȫ��ģʽ��fullscreen enabled

	// Set true to use 4X MSAA (?.1.8).  The default is false.
    bool      m4xMsaaState = false;    // 4X MSAA �Ƿ���Ч��enabled
    UINT      m4xMsaaQuality = 0;      // 4X MSAA�������ȼ�

	// Used to keep track of the delta-time and game time (?.4).
	GameTimer mTimer;		// ������¼֡����ʱ����Ϸʱ��
	
    Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;		// DX�����ӿ�
    Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;		// ������
    Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;		// D3D�豸

    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;				// Χ��
    UINT64 mCurrentFence = 0;								// ��ǰΧ��ֵ
	
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;			// �������
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc; // ֱ�������б������
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;		// �����б�

	static const int SwapChainBufferCount = 2;		// �������Ļ��������Ĭ��Ϊ2��
	int mCurrBackBuffer = 0;						// ��ǰ�󱸻�������
    Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];		// �������Ļ�������
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;		// ���ģ�建��

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;			// ��ȾĿ��������
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;			// ���ģ��������

    D3D12_VIEWPORT mScreenViewport; 		// ��Ļ�ӿ�
    D3D12_RECT mScissorRect;				// �ü�����

	UINT mRtvDescriptorSize = 0;			// ��ȾĿ�������ߴ�
	UINT mDsvDescriptorSize = 0;			// ���ģ�������ߴ�
	UINT mCbvSrvUavDescriptorSize = 0;		// ����/��ɫ����Դ/������������ߴ�

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring mMainWndCaption = L"d3d App";		// �����ڱ���
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;			// �豸���ͣ�Ӳ���豸
    DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;			// �󱸻����ʽ��RGBA8888
    DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;	// ���ģ�建���ʽ�����24λ��ģ��8λ
	int mClientWidth = 800;			// ���ڿ�
	int mClientHeight = 600;		// ���ڸ�
};

