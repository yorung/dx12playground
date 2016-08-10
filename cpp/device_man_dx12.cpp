#include "stdafx.h"

#ifdef __d3d12_h__

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

DeviceManDX12 deviceMan;

DeviceManDX12::FrameResources::~FrameResources()
{
	assert(!renderTarget);
	assert(!commandAllocator);
	assert(!constantBuffer);
	assert(!srvHeap);
	assert(!mappedConstantBuffer);
}

DeviceManDX12::~DeviceManDX12()
{
	assert(!factory);
	assert(!device);
	assert(!commandQueue);
	assert(!commandList);
	assert(!fence);
	assert(!swapChain);
	assert(!rtvHeap);
	assert(!depthStencil);
	assert(!dsvHeap);
}

void DeviceManDX12::Destroy()
{
	Flush();
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
		res.fenceValueToGuard = 0;
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

void DeviceManDX12::SetRenderTarget()
{
	DXGI_SWAP_CHAIN_DESC desc;
	swapChain->GetDesc(&desc);

	D3D12_VIEWPORT vp = { 0.f, 0.f, (float)desc.BufferDesc.Width, (float)desc.BufferDesc.Height, 0.f, 1.f };
	D3D12_RECT rc = { 0, 0, (LONG)desc.BufferDesc.Width, (LONG)desc.BufferDesc.Height };
	commandList->RSSetViewports(1, &vp);
	commandList->RSSetScissorRects(1, &rc);

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
	numAssignedSrvs = 0;
	numAssignedConstantBufferBlocks = 0;
	frameIndex = swapChain->GetCurrentBackBufferIndex();
	FrameResources& res = frameResources[frameIndex];
	afWaitFenceValue(fence, res.fenceValueToGuard);

	res.intermediateCommandlistDependentResources.clear();

	res.commandAllocator->Reset();
	commandList->Reset(res.commandAllocator.Get(), nullptr);
	ID3D12DescriptorHeap* ppHeaps[] = { res.srvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ res.renderTarget.Get(), 0, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET } };
	commandList->ResourceBarrier(1, &barrier);
}

void DeviceManDX12::EndScene()
{
	FrameResources& res = frameResources[frameIndex];
	D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ res.renderTarget.Get(), 0, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT } };
	commandList->ResourceBarrier(1, &barrier);

	commandList->Close();
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(lists), lists);

	commandQueue->Signal(fence.Get(), res.fenceValueToGuard = fenceValue++);
}

void DeviceManDX12::Flush()
{
	if (!commandList) {
		return;
	}
	commandList->Close();
	ID3D12CommandList* lists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(lists), lists);
	commandQueue->Signal(fence.Get(), fenceValue);
	afWaitFenceValue(fence, fenceValue++);

	for (FrameResources& res : frameResources)
	{
		res.commandAllocator->Reset();
		res.intermediateCommandlistDependentResources.clear();
	}
	FrameResources& res = frameResources[frameIndex];
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
	D3D12_RESOURCE_DESC resDesc = resToSrv->GetDesc();
	if (resDesc.DepthOrArraySize == 6) {
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = resDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.TextureCube.MipLevels = resDesc.MipLevels;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.ResourceMinLODClamp = 0;
		device->CreateShaderResourceView(resToSrv.Get(), &srvDesc, ptr);
	} else {
		device->CreateShaderResourceView(resToSrv.Get(), nullptr, ptr);
	}
}

int DeviceManDX12::AssignConstantBuffer(const void* buf, int size)
{
	int sizeAligned = (size + 0xff) & ~0xff;
	int numRequired = sizeAligned / 0x100;

	if (numAssignedConstantBufferBlocks + numRequired > maxConstantBufferBlocks) {
		assert(0);
		return -1;
	}
	int top = numAssignedConstantBufferBlocks;
	numAssignedConstantBufferBlocks += numRequired;

	FrameResources& res = frameResources[frameIndex];
	memcpy(res.mappedConstantBuffer + top, buf, size);
	return top;
}

void DeviceManDX12::AssignCBV(int descriptorHeapIndex, int constantBufferTop, int size)
{
	int sizeAligned = (size + 0xff) & ~0xff;
	FrameResources& res = frameResources[frameIndex];

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = res.constantBuffer->GetGPUVirtualAddress() + constantBufferTop * 0x100;
	cbvDesc.SizeInBytes = sizeAligned;
	assert((cbvDesc.SizeInBytes & 0xff) == 0);

	D3D12_CPU_DESCRIPTOR_HANDLE ptr = res.srvHeap->GetCPUDescriptorHandleForHeapStart();
	ptr.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * descriptorHeapIndex;
	device->CreateConstantBufferView(&cbvDesc, ptr);
}

void DeviceManDX12::AssignCBV(int descriptorHeapIndex, ComPtr<ID3D12Resource> ubo)
{
	D3D12_RESOURCE_DESC desc = ubo->GetDesc();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = ubo->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (UINT)desc.Width;
	assert((cbvDesc.SizeInBytes & 0xff) == 0);

	FrameResources& res = frameResources[frameIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE ptr = res.srvHeap->GetCPUDescriptorHandleForHeapStart();
	ptr.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * descriptorHeapIndex;
	device->CreateConstantBufferView(&cbvDesc, ptr);
}

void DeviceManDX12::AssignCBVAndConstantBuffer(int descriptorHeapIndex, const void* buf, int size)
{
	int top = AssignConstantBuffer(buf, size);
	if (top < 0) {
		assert(0);
		return;
	}
	AssignCBV(descriptorHeapIndex, top, size);
}

void DeviceManDX12::SetAssignedDescriptorHeap(int descriptorHeapIndex)
{
	FrameResources& res = frameResources[frameIndex];
	D3D12_GPU_DESCRIPTOR_HANDLE addr = res.srvHeap->GetGPUDescriptorHandleForHeapStart();
	addr.ptr += descriptorHeapIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	commandList->SetGraphicsRootDescriptorTable(0, addr);
}

void DeviceManDX12::AddIntermediateCommandlistDependentResource(ComPtr<ID3D12Resource> intermediateResource)
{
	FrameResources& res = frameResources[frameIndex];
	res.intermediateCommandlistDependentResources.push_back(intermediateResource);
}

void DeviceManDX12::Present()
{
	if (!swapChain) {
		return;
	}
	EndScene();
	swapChain->Present(0, 0);
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
		if (S_OK != swapChain->GetBuffer(i, IID_PPV_ARGS(&res.renderTarget))) {
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
		res.constantBuffer->SetName(L"DeviceMan constant buffer");
		D3D12_RANGE readRange = {};
		HRESULT hr = res.constantBuffer->Map(0, &readRange, (void**)&res.mappedConstantBuffer);
		assert(hr == S_OK);
		assert(res.mappedConstantBuffer);
	}

	IVec2 size = { (int)sd.BufferDesc.Width, (int)sd.BufferDesc.Height };
	depthStencil = afCreateTexture2D(AFDT_DEPTH_STENCIL, size, nullptr, true);

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

#endif
