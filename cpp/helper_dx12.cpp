#include "stdafx.h"

#ifdef __d3d12_h__

static constexpr D3D12_HEAP_PROPERTIES defaultHeapProperties = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
static constexpr D3D12_HEAP_PROPERTIES uploadHeapProperties = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
static constexpr float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static constexpr D3D12_RESOURCE_STATES TEXTURE_STATE = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

void afTransition(ID3D12GraphicsCommandList* cmd, ComPtr<ID3D12Resource> res, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to)
{
	cmd->ResourceBarrier(1, ToPtr<D3D12_RESOURCE_BARRIER>({ D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ res.Get(), 0, from, to } }));
}

void afSetPipeline(ComPtr<ID3D12PipelineState> ps, ComPtr<ID3D12RootSignature> rs)
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	list->SetPipelineState(ps.Get());
	list->SetGraphicsRootSignature(rs.Get());
}

void afSetDescriptorHeap(ComPtr<ID3D12DescriptorHeap> heap)
{
	ID3D12DescriptorHeap* ppHeaps[] = { heap.Get() };
	deviceMan.GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	deviceMan.GetCommandList()->SetGraphicsRootDescriptorTable(0, heap->GetGPUDescriptorHandleForHeapStart());
}

void afSetVertexBuffer(AFBufferResource id, int stride)
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	D3D12_RESOURCE_DESC desc = id->GetDesc();
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = { id->GetGPUVirtualAddress(), (UINT)desc.Width, (UINT)stride };
	list->IASetVertexBuffers(0, 1, &vertexBufferView);
}

void afSetVertexBuffers(int numIds, AFBufferResource ids[], int strides[])
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	D3D12_VERTEX_BUFFER_VIEW views[10];
	assert(numIds < _countof(views));
	for (int i = 0; i < numIds; i++)
	{
		D3D12_RESOURCE_DESC desc = ids[i]->GetDesc();
		views[i] = { ids[i]->GetGPUVirtualAddress(), (UINT)desc.Width, (UINT)strides[i] };
	}

	list->IASetVertexBuffers(0, numIds, views);
}

void afSetIndexBuffer(AFBufferResource id)
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	D3D12_RESOURCE_DESC desc = id->GetDesc();
	D3D12_INDEX_BUFFER_VIEW indexBufferView = { id->GetGPUVirtualAddress(), (UINT)desc.Width, AFIndexTypeToDevice };
	list->IASetIndexBuffer(&indexBufferView);
}

void afWriteBuffer(const AFBufferResource id, int size, const void* buf)
{
#ifdef _DEBUG
	D3D12_RESOURCE_DESC desc = id->GetDesc();
	if (size > (int)desc.Width)
	{
		return;
	}
#endif
	void* p;
	D3D12_RANGE readRange = {};
	afHandleDXError(id->Map(0, &readRange, &p));
	assert(p);
	memcpy(p, buf, size);
	D3D12_RANGE wroteRange = { 0, (SIZE_T)size };
	id->Unmap(0, &wroteRange);
}

static D3D12_RESOURCE_STATES BufferTypeToResourceState(AFBufferType bufferType)
{
	switch (bufferType)
	{
	case AFBT_VERTEX:
	case AFBT_VERTEX_CPUWRITE:
	case AFBT_CONSTANT:
	case AFBT_CONSTANT_CPUWRITE:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	case AFBT_INDEX:
		return D3D12_RESOURCE_STATE_INDEX_BUFFER;
	}
	assert(0);
	return D3D12_RESOURCE_STATE_COMMON;
}

static ComPtr<ID3D12Resource> afCreateUploadHeap(int size)
{
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, (UINT64)size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	AFBufferResource o;
	afHandleDXError(deviceMan.GetDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&o)));
	return o;
}

static ComPtr<ID3D12Resource> afCreateFixedBuffer(int size, const void* buf, AFBufferType bufferType)
{
	assert(buf);
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, (UINT64)size, 1, 1, 1, DXGI_FORMAT_UNKNOWN,{ 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	AFBufferResource o;
	afHandleDXError(deviceMan.GetDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&o)));
	if (o)
	{
		ComPtr<ID3D12Resource> intermediateBuffer = afCreateUploadHeap(size);
		afWriteBuffer(intermediateBuffer, size, buf);
		ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
		list->CopyBufferRegion(o.Get(), 0, intermediateBuffer.Get(), 0, size);
		afTransition(list, o, D3D12_RESOURCE_STATE_COPY_DEST, BufferTypeToResourceState(bufferType));
		deviceMan.AddIntermediateCommandlistDependentResource(intermediateBuffer);
		deviceMan.AddIntermediateCommandlistDependentResource(o);
	}
	return o;
}

ComPtr<ID3D12Resource> afCreateBuffer(int size, const void* buf, AFBufferType bufferType)
{
	switch (bufferType)
	{
	case AFBT_VERTEX:
	case AFBT_INDEX:
	case AFBT_CONSTANT:
		return afCreateFixedBuffer(size, buf, bufferType);
	case AFBT_VERTEX_CPUWRITE:
	case AFBT_CONSTANT_CPUWRITE:
	{
		ComPtr<ID3D12Resource> o = afCreateUploadHeap((size + 0xff) & ~0xff);
		if (buf)
		{
			afWriteBuffer(o, size, buf);
		}
		return o;
	}
	}
	assert(0);
	return nullptr;
}

static ComPtr<ID3D12Resource> afCreateBuffer(int size, const void* buf = nullptr)
{
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, (UINT64)size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	AFBufferResource o;
	HRESULT hr = deviceMan.GetDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&o));
	assert(hr == S_OK);
	if (buf)
	{
		afWriteBuffer(o, size, buf);
	}
	return o;
}

AFBufferResource afCreateDynamicVertexBuffer(int size, const void* buf)
{
	return afCreateBuffer(size, buf);
}

AFBufferResource afCreateVertexBuffer(int size, const void* buf)
{
	return afCreateBuffer(size, buf, AFBT_VERTEX);
}

AFBufferResource afCreateIndexBuffer(const AFIndex* indi, int numIndi)
{
	assert(indi);
	int size = numIndi * sizeof(AFIndex);
	return afCreateBuffer(size, indi, AFBT_INDEX);
}

AFBufferResource afCreateUBO(int size)
{
	return afCreateBuffer((size + 0xff) & ~0xff);
}

void afWriteTexture(SRVID tex, const TexDesc& desc, int mipCount, const AFTexSubresourceData datas[])
{
	const int maxSubresources = 100;
	const UINT subResources = mipCount * desc.arraySize;
	assert(subResources <= maxSubresources);
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[maxSubresources];
	UINT64 rowSizeInBytes[maxSubresources], uploadSize;
	UINT numRows[maxSubresources];
	D3D12_RESOURCE_BARRIER transitions1[maxSubresources], transitions2[maxSubresources];
	deviceMan.GetDevice()->GetCopyableFootprints(&tex->GetDesc(), 0, subResources, 0, footprints, numRows, rowSizeInBytes, &uploadSize);
	ComPtr<ID3D12Resource> uploadBuf = afCreateBuffer((int)uploadSize);
	assert(uploadBuf);
	uploadBuf->SetName(__FUNCTIONW__ L" intermediate buffer");
	D3D12_RANGE readRange = {};
	BYTE* ptr;
	HRESULT hr = uploadBuf->Map(0, &readRange, (void**)&ptr);
	assert(ptr);
	for (UINT i = 0; i < subResources; i++)
	{
		transitions1[i] = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ tex.Get(), i, TEXTURE_STATE, D3D12_RESOURCE_STATE_COPY_DEST } };
		transitions2[i] = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ tex.Get(), i, D3D12_RESOURCE_STATE_COPY_DEST, TEXTURE_STATE } };
	}
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	list->ResourceBarrier(subResources, transitions1);
	for (UINT i = 0; i < subResources; i++)
	{
		assert(datas[i].pitch == rowSizeInBytes[i]);
		assert(datas[i].pitch <= footprints[i].Footprint.RowPitch);
		for (UINT row = 0; row < numRows[i]; row++)
		{
			memcpy(ptr + footprints[i].Offset + footprints[i].Footprint.RowPitch * row, (BYTE*)datas[i].ptr + datas[i].pitch * row, datas[i].pitch);
		}
		D3D12_TEXTURE_COPY_LOCATION uploadBufLocation = { uploadBuf.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, footprints[i] };
		D3D12_TEXTURE_COPY_LOCATION nativeBufLocation = { tex.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i };
		list->CopyTextureRegion(&nativeBufLocation, 0, 0, 0, &uploadBufLocation, nullptr);
	}
	list->ResourceBarrier(subResources, transitions2);
	uploadBuf->Unmap(0, nullptr);
	deviceMan.AddIntermediateCommandlistDependentResource(uploadBuf);
	deviceMan.AddIntermediateCommandlistDependentResource(tex);
}

void afWriteTexture(SRVID id, const TexDesc& desc, const void* buf)
{
	assert(desc.arraySize == 1);
	assert(!desc.isCubeMap);

	const D3D12_RESOURCE_DESC destDesc = id->GetDesc();
	assert(destDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM);
	AFTexSubresourceData data = { buf, (uint32_t)desc.size.x * 4, 0 };
	afWriteTexture(id, desc, 1, &data);
}

void afSetTextureName(SRVID tex, const char* name)
{
	if (tex)
	{
		tex->SetName(SWPrintf(L"%S", name));
	}
}

ComPtr<ID3D12Resource> afCreateDynamicTexture(AFFormat format, const IVec2& size, uint32_t flags)
{
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;
	textureDesc.Width = size.x;
	textureDesc.Height = size.y;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	bool useClearValue = !!(flags & (AFTF_DSV | AFTF_RTV));
	D3D12_CLEAR_VALUE clearValue = { afTypelessToDSVFormat(format) };
	D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
	if (flags & AFTF_SRV)
	{
		resourceState = TEXTURE_STATE;
	}
	if (flags & AFTF_DSV)
	{
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		clearValue.DepthStencil.Depth = 1.0f;
		resourceState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}
	if (flags & AFTF_RTV)
	{
		textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		std::copy_n(clearColor, 4, clearValue.Color);
		resourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;	// not |=
	}

	ComPtr<ID3D12Resource> res;
	afHandleDXError(deviceMan.GetDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, resourceState, useClearValue ? &clearValue : nullptr, IID_PPV_ARGS(&res)));
	return res;
}

SRVID afCreateTexture2D(AFFormat format, const IVec2& size, void *image, bool isRenderTargetOrDepthStencil)
{
	bool isDepthStencil = format == AFF_D32_FLOAT || format == AFF_D32_FLOAT_S8_UINT || format == AFF_D24_UNORM_S8_UINT;
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = format;
	textureDesc.Width = size.x;
	textureDesc.Height = size.y;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	SRVID id;
	D3D12_CLEAR_VALUE clearValue = { format };

	if (isRenderTargetOrDepthStencil)
	{
		textureDesc.Flags = isDepthStencil ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		if (isDepthStencil)
		{
			textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			clearValue.DepthStencil.Depth = 1.0f;
		}
		else
		{
			textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			std::copy_n(clearColor, 4, clearValue.Color);
		}
	}

	HRESULT hr = deviceMan.GetDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, isRenderTargetOrDepthStencil ? &clearValue : nullptr, IID_PPV_ARGS(&id));
	TexDesc texDesc;
	texDesc.size = size;
	if (image)
	{
		afWriteTexture(id, texDesc, image);
	}
	return id;
}

SRVID afCreateTexture2D(AFFormat format, const struct TexDesc& desc, int mipCount, const AFTexSubresourceData datas[])
{
	const D3D12_RESOURCE_DESC resourceDesc = { D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, (UINT64)desc.size.x, (UINT)desc.size.y, (UINT16)desc.arraySize, (UINT16)mipCount, format, {1, 0} };
	SRVID tex;
	afHandleDXError(deviceMan.GetDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, TEXTURE_STATE, nullptr, IID_PPV_ARGS(&tex)));
	afWriteTexture(tex, desc, mipCount, datas);
	return tex;
}

void afDrawIndexed(PrimitiveTopology pt, int numIndices, int start, int instanceCount)
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	list->IASetPrimitiveTopology(pt);
	list->DrawIndexedInstanced(numIndices, instanceCount, start, 0, 0);
}

void afDraw(PrimitiveTopology pt, int numVertices, int start, int instanceCount)
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	list->IASetPrimitiveTopology(pt);
	list->DrawInstanced(numVertices, instanceCount, start, 0);
}

#include <D3Dcompiler.h>
ComPtr<ID3D12PipelineState> afCreatePSO(const char *shaderName, const InputElement elements[], int numElements, BlendMode blendMode, DepthStencilMode depthStencilMode, CullMode cullMode, ComPtr<ID3D12RootSignature>& rootSignature, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology)
{
	ComPtr<ID3DBlob> vertexShader = afCompileHLSL(shaderName, "VSMain", "vs_5_0");
	ComPtr<ID3DBlob> pixelShader = afCompileHLSL(shaderName, "PSMain", "ps_5_0");
	ComPtr<ID3DBlob> rootSignatureBlob;
	if (S_OK == D3DGetBlobPart(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize(), D3D_BLOB_ROOT_SIGNATURE, 0, &rootSignatureBlob))
	{
		if (rootSignatureBlob)
		{
			HRESULT hr = deviceMan.GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
		}
	}

	static constexpr D3D12_RENDER_TARGET_BLEND_DESC solid = {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
	};
	static constexpr D3D12_RENDER_TARGET_BLEND_DESC alphaBlend = {
		TRUE, FALSE,
		D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	switch (blendMode)
	{
	case BM_NONE:
		psoDesc.BlendState.RenderTarget[0] = solid;
		break;
	case BM_ALPHA:
		psoDesc.BlendState.RenderTarget[0] = alphaBlend;
		break;
	}
	psoDesc.InputLayout = { elements, (UINT)numElements };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
	psoDesc.RasterizerState = { D3D12_FILL_MODE_SOLID, cullMode == CM_CCW ? D3D12_CULL_MODE_FRONT : cullMode == CM_CW ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE };
	psoDesc.DepthStencilState = { depthStencilMode != DSM_DISABLE, D3D12_DEPTH_WRITE_MASK_ALL, depthStencilMode == DSM_DEPTH_CLOSEREQUAL_READONLY ? D3D12_COMPARISON_FUNC_LESS_EQUAL : D3D12_COMPARISON_FUNC_LESS };
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = primitiveTopology;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = AFF_D32_FLOAT_S8_UINT;
	psoDesc.SampleDesc.Count = 1;
	ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = deviceMan.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	assert(hr == S_OK);
	return pso;
}

class CDescriptorCBV : public D3D12_DESCRIPTOR_RANGE
{
public:
	CDescriptorCBV(int shaderRegister)
	{
		RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		NumDescriptors = 1;
		BaseShaderRegister = shaderRegister;
		RegisterSpace = 0;
		OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}
};

class CDescriptorSRV : public D3D12_DESCRIPTOR_RANGE
{
public:
	CDescriptorSRV(int shaderRegister)
	{
		RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		NumDescriptors = 1;
		BaseShaderRegister = shaderRegister;
		RegisterSpace = 0;
		OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}
};

ComPtr<ID3D12DescriptorHeap> afCreateDescriptorHeap(int numSrvs, SRVID srvs[])
{
	ComPtr<ID3D12DescriptorHeap> heap;
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = numSrvs;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = deviceMan.GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&heap));
	assert(hr == S_OK);
	for (int i = 0; i < numSrvs; i++)
	{
		D3D12_RESOURCE_DESC desc = srvs[i]->GetDesc();
		auto ptr = heap->GetCPUDescriptorHandleForHeapStart();
		ptr.ptr += deviceMan.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * i;
		if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = srvs[i]->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (UINT)desc.Width;
			assert((cbvDesc.SizeInBytes & 0xff) == 0);
			deviceMan.GetDevice()->CreateConstantBufferView(&cbvDesc, ptr);
		}
		else
		{
			deviceMan.GetDevice()->CreateShaderResourceView(srvs[i].Get(), nullptr, ptr);
		}
	}

	return heap;
}

void afWaitFenceValue(ComPtr<ID3D12Fence> fence, UINT64 value)
{
	if (fence->GetCompletedValue() >= value)
	{
		return;
	}
	HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(fenceEvent);
	fence->SetEventOnCompletion(value, fenceEvent);
	WaitForSingleObject(fenceEvent, INFINITE);
	CloseHandle(fenceEvent);
}

IVec2 afGetTextureSize(SRVID tex)
{
	D3D12_RESOURCE_DESC desc = tex->GetDesc();
	return IVec2((int)desc.Width, (int)desc.Height);
}

void afBindTextureToBindingPoint(SRVID srv, int rootParameterIndex)
{
	int descriptorHeapIndex = deviceMan.AssignDescriptorHeap(1);
	deviceMan.AssignSRV(descriptorHeapIndex, srv);
	deviceMan.SetAssignedDescriptorHeap(descriptorHeapIndex, rootParameterIndex);
}

void afSetVertexBufferFromSystemMemory(const void* buf, int size, int stride)
{
	AFBufferResource vbo = afCreateDynamicVertexBuffer(size, buf);
	afSetVertexBuffer(vbo, stride);
	deviceMan.AddIntermediateCommandlistDependentResource(vbo);
}

void AFDynamicQuadListVertexBuffer::Create(const InputElement*, int, int vertexSize_, int nQuad)
{
	Destroy();
	stride = vertexSize_;
	vertexBufferSize = nQuad * vertexSize_ * 4;
	ibo = afCreateQuadListIndexBuffer(nQuad);
}

void AFDynamicQuadListVertexBuffer::Write(const void* buf, int size)
{
	assert(size <= vertexBufferSize);
	AFBufferResource vbo = afCreateDynamicVertexBuffer(size, buf);
	afSetVertexBuffer(vbo, stride);
	deviceMan.AddIntermediateCommandlistDependentResource(vbo);
}

void AFRenderTarget::InitForDefaultRenderTarget()
{
	asDefault = true;
}

void AFRenderTarget::Init(IVec2 size, AFFormat colorFormat, AFFormat depthStencilFormat)
{
	texSize = size;
	currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	renderTarget = afCreateDynamicTexture(colorFormat, size, AFTF_RTV | AFTF_SRV);
	deviceMan.AddIntermediateCommandlistDependentResource(renderTarget);
	afSetTextureName(renderTarget, __FUNCTION__);
	ID3D12GraphicsCommandList* commandList = deviceMan.GetCommandList();
}

void AFRenderTarget::Destroy()
{
	renderTarget.Reset();
}

void AFRenderTarget::BeginRenderToThis()
{
	ID3D12GraphicsCommandList* commandList = deviceMan.GetCommandList();
	if (currentState != D3D12_RESOURCE_STATE_RENDER_TARGET)
	{
		afTransition(commandList, renderTarget, currentState, D3D12_RESOURCE_STATE_RENDER_TARGET);
		currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	if (asDefault)
	{
		deviceMan.SetRenderTarget();
		return;
	}

	D3D12_VIEWPORT vp = { 0.f, 0.f, (float)texSize.x, (float)texSize.y, 0.f, 1.f };
	D3D12_RECT rc = { 0, 0, (LONG)texSize.x, (LONG)texSize.y };
	commandList->RSSetViewports(1, &vp);
	commandList->RSSetScissorRects(1, &rc);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = deviceMan.GetDepthStencilView();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = deviceMan.GetRenderTargetView();
	deviceMan.GetDevice()->CreateRenderTargetView(renderTarget.Get(), nullptr, rtvHandle);

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

ComPtr<ID3D12Resource> AFRenderTarget::GetTexture()
{
	if (currentState != TEXTURE_STATE)
	{
		ID3D12GraphicsCommandList* commandList = deviceMan.GetCommandList();
		afTransition(commandList, renderTarget, currentState, TEXTURE_STATE);
		currentState = TEXTURE_STATE;
	}

	return renderTarget;
}

FakeVAO::FakeVAO(int numBuffers, AFBufferResource const vbos_[], const int strides_[], const UINT offsets_[], AFBufferResource ibo_)
{
	ibo = ibo_;
	vbos.resize(numBuffers);
	strides.resize(numBuffers);
	offsets.resize(numBuffers);
	for (int i = 0; i < numBuffers; i++)
	{
		vbos[i] = vbos_[i];
		strides[i] = (UINT)strides_[i];
		offsets[i] = offsets_ ? offsets_[i] : 0;
	}
}

void FakeVAO::Apply()
{
	afSetVertexBuffers((int)vbos.size(), vbos.data(), strides.data());
	if (ibo)
	{
		afSetIndexBuffer(ibo);
	}
}

void afBindCbvs(AFCbvBindToken cbvs[], int nCbvs)
{
	for (int i = 0; i < nCbvs; i++)
	{
		AFCbvBindToken& cbv = cbvs[i];
		if (cbv.top >= 0)
		{
			deviceMan.GetCommandList()->SetGraphicsRootConstantBufferView(i, deviceMan.GetConstantBufferGPUAddress(cbv.top));
		}
		else if (cbv.ubo)
		{
			afBindBufferToBindingPoint(cbv.ubo, i);
		}
		else
		{
			assert(0);
		}
	}
}

void afBindBufferToBindingPoint(const void* buf, int size, int rootParameterIndex)
{
	int cbTop = deviceMan.AssignConstantBuffer(buf, size);
	deviceMan.GetCommandList()->SetGraphicsRootConstantBufferView(rootParameterIndex, deviceMan.GetConstantBufferGPUAddress(cbTop));
}

void afBindBufferToBindingPoint(AFBufferResource ubo, int rootParameterIndex)
{
	deviceMan.GetCommandList()->SetGraphicsRootConstantBufferView(rootParameterIndex, ubo->GetGPUVirtualAddress());
}

VAOID afCreateVAO(const InputElement elements[], int numElements, int numBuffers, AFBufferResource const vertexBufferIds[], const int strides[], AFBufferResource ibo)
{
	(void)elements;
	(void)numElements;
	VAOID p(new FakeVAO(numBuffers, vertexBufferIds, strides, nullptr, ibo));
	return p;
}

void afBindVAO(const VAOID& vao)
{
	if (vao)
	{
		vao->Apply();
	}
}

#endif
