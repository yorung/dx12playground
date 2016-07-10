#include "stdafx.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

DeviceManDX12 deviceMan;

DeviceManDX12::~DeviceManDX12()
{
	assert(!factory);
	assert(!device);
	assert(!commandQueue);
	assert(!commandAllocator);
	assert(!commandList);
	assert(!fence);
	assert(!swapChain);
	for (int i = 0; i < numFrameBuffers; i++) {
		assert(!renderTargets[i]);
	}
	assert(!rtvHeap);
	assert(!depthStencil);
	assert(!dsvHeap);
	assert(!srvHeap);
	assert(!constantBuffer);
}

void DeviceManDX12::Destroy()
{
	WaitForPreviousFrame();
	commandList.Reset();
	commandQueue.Reset();
	swapChain.Reset();
	for (FrameResources& res : frameResources) {
		res.renderTarget.Reset();
		res.commandAllocator.Reset();
		res.srvHeap.Reset();
		res.mappedConstantBuffer = nullptr;
		if (res.constantBuffer) {
			res.constantBuffer->Unmap(0, nullptr);
		}
		res.constantBuffer.Reset();
	}
	rtvHeap.Reset();
	depthStencil.Reset();
	dsvHeap.Reset();
	factory.Reset();
	fence.Reset();
	fenceValue = 1;
	frameIndex = 0;
	int cnt = device.Reset();
	if (cnt) {
		MessageBoxA(GetActiveWindow(), SPrintf("%d leaks detected.", cnt), "DX12 leaks", MB_OK);
	}
}

void DeviceManDX12::WaitForPreviousFrame()
{
	if (!commandQueue) {
		return;
	}
	commandQueue->Signal(fence.Get(), fenceValue);
	WaitFenceValue(fenceValue++);
}

void DeviceManDX12::WaitFenceValue(UINT64 value)
{
	assert(fence);
	if (fence->GetCompletedValue() < value) {
		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		assert(fenceEvent);
		fence->SetEventOnCompletion(value, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);
	}
}

void DeviceManDX12::SetRenderTarget()
{
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += frameIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	const float clearColor[] = { 0.0f, 0.2f, 0.3f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

void DeviceManDX12::BeginScene()
{
	WaitForPreviousFrame();
	numAssignedSrvs = 0;
	numAssignedConstantBufferBlocks = 0;
	frameIndex = deviceMan.GetSwapChain()->GetCurrentBackBufferIndex();
	FrameResources& res = frameResources[frameIndex];
	res.commandAllocator->Reset();
	commandList->Reset(res.commandAllocator.Get(), nullptr);
	D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ deviceMan.GetRenderTarget().Get(), 0, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET } };
	commandList->ResourceBarrier(1, &barrier);

	DXGI_SWAP_CHAIN_DESC desc;
	deviceMan.GetSwapChain()->GetDesc(&desc);

	D3D12_VIEWPORT vp = {0.f, 0.f, (float)desc.BufferDesc.Width, (float)desc.BufferDesc.Height, 0.f, 1.f};
	D3D12_RECT rc = {0, 0, (LONG)desc.BufferDesc.Width, (LONG)desc.BufferDesc.Height};
	commandList->RSSetViewports(1, &vp);
	commandList->RSSetScissorRects(1, &rc);

	SetRenderTarget();
}

void DeviceManDX12::EndScene()
{
	D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ deviceMan.GetRenderTarget().Get(), 0, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT } };
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(lists), lists);
}

void DeviceManDX12::Flush()
{
	if (!commandList) {
		return;
	}
	commandList->Close();
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(lists), lists);
	WaitForPreviousFrame();

	FrameResources& res = frameResources[frameIndex];
	res.commandAllocator->Reset();
	commandList->Reset(res.commandAllocator.Get(), nullptr);
}

int DeviceManDX12::AssignDescriptorHeap(int numRequired)
{
	if (numAssignedSrvs + numRequired > maxSrvs) {
		assert(0);
		return -1;
	}
	int head = numAssignedSrvs;
	numAssignedSrvs = numAssignedSrvs + numRequired;
	return head;
}

void DeviceManDX12::AssignSRV(int descriptorHeapIndex, ComPtr<ID3D12Resource> resToSrv)
{
	FrameResources& res = frameResources[frameIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE ptr = res.srvHeap->GetCPUDescriptorHandleForHeapStart();
	ptr.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * descriptorHeapIndex;
	device->CreateShaderResourceView(resToSrv.Get(), nullptr, ptr);
}

void DeviceManDX12::AssignConstantBuffer(int descriptorHeapIndex, const void* buf, int size)
{
	int sizeAligned = (size + 0xff) & ~0xff;
	int numRequired = sizeAligned / 0x100;

	if (numAssignedConstantBufferBlocks + numRequired > maxConstantBufferBlocks) {
		assert(0);
		return;
	}
	int top = numAssignedConstantBufferBlocks;
	numAssignedConstantBufferBlocks += numRequired;

	FrameResources& res = frameResources[frameIndex];

	memcpy(res.mappedConstantBuffer + top, buf, size);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = res.constantBuffer->GetGPUVirtualAddress() + top * 0x100;
	cbvDesc.SizeInBytes = sizeAligned;
	assert((cbvDesc.SizeInBytes & 0xff) == 0);

	D3D12_CPU_DESCRIPTOR_HANDLE ptr = res.srvHeap->GetCPUDescriptorHandleForHeapStart();
	ptr.ptr += deviceMan.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * descriptorHeapIndex;
	deviceMan.GetDevice()->CreateConstantBufferView(&cbvDesc, ptr);
}

void DeviceManDX12::SetAssignedDescriptorHeap(int descriptorHeapIndex)
{
	FrameResources& res = frameResources[frameIndex];
	ID3D12DescriptorHeap* ppHeaps[] = { res.srvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	D3D12_GPU_DESCRIPTOR_HANDLE addr = res.srvHeap->GetGPUDescriptorHandleForHeapStart();
	addr.ptr += descriptorHeapIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(0, addr);
}

void DeviceManDX12::Present()
{
	if (!swapChain) {
		return;
	}
	EndScene();
	swapChain->Present(1, 0);
	BeginScene();
}

void DeviceManDX12::Create(HWND hWnd)
{
	Destroy();
#ifndef NDEBUG
	ComPtr<ID3D12Debug> debug;
	if (S_OK == D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))
	{
		debug->EnableDebugLayer();
	}
#endif
	if (S_OK != CreateDXGIFactory1(IID_PPV_ARGS(&factory))) {
		Destroy();
		return;
	}
	ComPtr<IDXGIAdapter1> adapter;
	for (int i = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(i, &adapter); i++) {
		if (S_OK == D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))) {
			break;
		}
	}
	if (!device) {
		Destroy();
		return;
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = 0;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.NodeMask = 0;
	if (S_OK != device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))) {
		Destroy();
		return;
	}

	RECT rc;
	GetClientRect(hWnd, &rc);

	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferCount = numFrameBuffers;
	sd.BufferDesc.Width = rc.right - rc.left;
	sd.BufferDesc.Height = rc.bottom - rc.top;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.Windowed = TRUE;

	ComPtr<IDXGISwapChain> sc;
	if (S_OK != factory->CreateSwapChain(commandQueue.Get(), &sd, &sc)) {
		Destroy();
		return;
	}
	if (S_OK != sc.As(&swapChain)) {
		Destroy();
		return;
	}

	const D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, numFrameBuffers };
	device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
	for (int i = 0; i < numFrameBuffers; i++) {
		FrameResources& res = frameResources[i];
		if (S_OK != deviceMan.GetSwapChain()->GetBuffer(i, IID_PPV_ARGS(&res.renderTarget))) {
			Destroy();
			return;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		device->CreateRenderTargetView(res.renderTarget.Get(), nullptr, rtvHandle);

		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&res.commandAllocator));

		const D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, maxSrvs, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
		device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&res.srvHeap));

		res.constantBuffer = afCreateUBO(maxConstantBufferBlocks * 0x100);
		D3D12_RANGE readRange = {};
		HRESULT hr = res.constantBuffer->Map(0, &readRange, (void**)&res.mappedConstantBuffer);
		assert(hr == S_OK);
		assert(res.mappedConstantBuffer);
	}

	IVec2 size = { (int)sd.BufferDesc.Width, (int)sd.BufferDesc.Height };
	depthStencil = afCreateTexture2D(AFDT_DEPTH_STENCIL, size);

	const D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1 };
	device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
	device->CreateDepthStencilView(depthStencil.Get(), nullptr, dsvHandle);

	factory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frameResources[0].commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	commandList->Close();

	if (S_OK != device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))) {
		Destroy();
		return;
	}
	BeginScene();
}
