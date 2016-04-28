class DeviceManDX12
{
	static const UINT numFrameBuffers = 2;
	ComPtr<IDXGIFactory4> factory;
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<ID3D12Resource> renderTargets[numFrameBuffers], depthStencil;
	ComPtr<ID3D12DescriptorHeap> rtvHeap, dsvHeap;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent = INVALID_HANDLE_VALUE;
	UINT64 fenceValue = 1;
	UINT frameIndex = 0;
	void BeginScene();
	void EndScene();
	void WaitForPreviousFrame();
	void SetRenderTarget();
public:
	~DeviceManDX12();
	void Create(HWND hWnd);
	void Destroy();
	void Present();
	void Flush();
	ComPtr<ID3D12Device> GetDevice() { return device; }
	ComPtr<IDXGIFactory4> GetFactory() { return factory; }
	ComPtr<IDXGISwapChain3> GetSwapChain() { return swapChain; }
	ComPtr<ID3D12Resource> GetRenderTarget() { return renderTargets[frameIndex]; }
	ID3D12CommandAllocator* GetCommandAllocator() { return commandAllocator.Get(); }
	ID3D12GraphicsCommandList* GetCommandList() { return commandList.Get(); }
};

extern DeviceManDX12 deviceMan;
