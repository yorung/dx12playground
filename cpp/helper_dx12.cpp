#include "stdafx.h"

#ifdef __d3d12_h__

static const D3D12_HEAP_PROPERTIES defaultHeapProperties = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
static const D3D12_HEAP_PROPERTIES uploadHeapProperties = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
static const float clearColor[] = { 0.0f, 0.2f, 0.3f, 1.0f };

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

void afSetVertexBuffer(VBOID id, int stride)
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	D3D12_RESOURCE_DESC desc = id->GetDesc();
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = { id->GetGPUVirtualAddress(), (UINT)desc.Width, (UINT)stride };
	list->IASetVertexBuffers(0, 1, &vertexBufferView);
}

void afSetVertexBuffers(int numIds, VBOID ids[], int strides[])
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	D3D12_VERTEX_BUFFER_VIEW views[10];
	assert(numIds < _countof(views));
	for (int i = 0; i < numIds; i++) {
		D3D12_RESOURCE_DESC desc = ids[i]->GetDesc();
		views[i] = { ids[i]->GetGPUVirtualAddress(), (UINT)desc.Width, (UINT)strides[i] };
	}

	list->IASetVertexBuffers(0, numIds, views);
}

void afSetIndexBuffer(IBOID id)
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	D3D12_RESOURCE_DESC desc = id->GetDesc();
	D3D12_INDEX_BUFFER_VIEW indexBufferView = { id->GetGPUVirtualAddress(), (UINT)desc.Width, AFIndexTypeToDevice };
	list->IASetIndexBuffer(&indexBufferView);
}

void afWriteBuffer(const IBOID id, const void* buf, int size)
{
#ifdef _DEBUG
	D3D12_RESOURCE_DESC desc = id->GetDesc();
	if (size > (int)desc.Width) {
		return;
	}
#endif
	void* p;
	D3D12_RANGE readRange = {};
	HRESULT hr = id->Map(0, &readRange, &p);
	assert(hr == S_OK);
	assert(p);
	memcpy(p, buf, size);
	D3D12_RANGE wroteRange = {0, (SIZE_T)size};
	id->Unmap(0, &wroteRange);
}

ComPtr<ID3D12Resource> afCreateBuffer(int size, const void* buf)
{
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, (UINT64)size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	UBOID o;
	HRESULT hr = deviceMan.GetDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&o));
	assert(hr == S_OK);
	if (buf) {
		afWriteBuffer(o, buf, size);
	}
	return o;
}

VBOID afCreateDynamicVertexBuffer(int size, const void* buf)
{
	return afCreateBuffer(size, buf);
}

VBOID afCreateVertexBuffer(int size, const void* buf)
{
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, (UINT64)size, 1, 1, 1, DXGI_FORMAT_UNKNOWN,{ 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	VBOID o;
	deviceMan.GetDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, nullptr, IID_PPV_ARGS(&o));
	if (o) {
		ComPtr<ID3D12Resource> intermediateBuffer = afCreateBuffer(size, buf);
		deviceMan.GetCommandList()->CopyBufferRegion(o.Get(), 0, intermediateBuffer.Get(), 0, size);
		deviceMan.AddIntermediateCommandlistDependentResource(intermediateBuffer);
		deviceMan.AddIntermediateCommandlistDependentResource(o);
	}
	return o;
}

IBOID afCreateIndexBuffer(const AFIndex* indi, int numIndi)
{
	assert(indi);
	int size = numIndi * sizeof(AFIndex);

	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, (UINT64)size, 1, 1, 1, DXGI_FORMAT_UNKNOWN,{ 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	IBOID o;
	deviceMan.GetDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_INDEX_BUFFER, nullptr, IID_PPV_ARGS(&o));
	if (o) {
		ComPtr<ID3D12Resource> intermediateBuffer = afCreateBuffer(size, indi);
		deviceMan.GetCommandList()->CopyBufferRegion(o.Get(), 0, intermediateBuffer.Get(), 0, size);
		deviceMan.AddIntermediateCommandlistDependentResource(intermediateBuffer);
		deviceMan.AddIntermediateCommandlistDependentResource(o);
	}
	return o;
}

UBOID afCreateUBO(int size)
{
	return afCreateBuffer((size + 0xff) & ~0xff);
}

void afWriteTexture(SRVID id, const TexDesc& desc, int mipCount, const AFTexSubresourceData datas[])
{
	const UINT subResources = mipCount * desc.arraySize;
	const D3D12_RESOURCE_DESC destDesc = id->GetDesc();
/*	for debug
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;
	footprints.resize(subResources);
	deviceMan.GetDevice()->GetCopyableFootprints(&destDesc, 0, subResources, 0, &footprints[0], nullptr, nullptr, nullptr);
*/


	for (UINT i = 0; i < subResources; i++) {
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
		UINT numRow;
		UINT64 rowSizeInBytes, uploadSize;
		deviceMan.GetDevice()->GetCopyableFootprints(&destDesc, i, 1, 0, &footprint, &numRow, &rowSizeInBytes, &uploadSize);
		ComPtr<ID3D12Resource> uploadBuf;
		if (datas[i].pitch == footprint.Footprint.RowPitch) {
			uploadBuf = afCreateBuffer((int)uploadSize, datas[i].ptr);
		} else {
			assert(datas[i].pitch == rowSizeInBytes);
			assert(datas[i].pitch < footprint.Footprint.RowPitch);
			uploadBuf = afCreateBuffer((int)uploadSize);
			D3D12_RANGE readRange = {};
			BYTE* ptr;
			HRESULT hr = uploadBuf->Map(0, &readRange, (void**)&ptr);
			assert(ptr);
			for (UINT row = 0; row < numRow; row++) {
				memcpy(ptr + footprint.Footprint.RowPitch * row, (BYTE*)datas[i].ptr + datas[i].pitch * row, datas[i].pitch);
			}
			uploadBuf->Unmap(0, nullptr);
		}
		uploadBuf->SetName(__FUNCTIONW__ L" intermediate buffer");
		D3D12_TEXTURE_COPY_LOCATION uploadBufLocation = { uploadBuf.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, footprint };
		D3D12_TEXTURE_COPY_LOCATION nativeBufLocation = { id.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i };
		ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();

		D3D12_RESOURCE_BARRIER transition1 = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ id.Get(), i, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST } };
		list->ResourceBarrier(1, &transition1);
		list->CopyTextureRegion(&nativeBufLocation, 0, 0, 0, &uploadBufLocation, nullptr);
		D3D12_RESOURCE_BARRIER transition2 = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE,{ id.Get(), i, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE } };
		list->ResourceBarrier(1, &transition2);

		deviceMan.AddIntermediateCommandlistDependentResource(uploadBuf);
	}

	deviceMan.AddIntermediateCommandlistDependentResource(id);
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

SRVID afCreateTexture2D(AFDTFormat format, const IVec2& size, void *image, bool isRenderTargetOrDepthStencil)
{
	bool isDepthStencil = format == AFDT_DEPTH || format == AFDT_DEPTH_STENCIL;
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

	if (isRenderTargetOrDepthStencil) {
		textureDesc.Flags = isDepthStencil ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		if (isDepthStencil) {
			textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			clearValue.DepthStencil.Depth = 1.0f;
		} else {
			textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			std::copy_n(clearColor, 4, clearValue.Color);
		}
	}

	HRESULT hr = deviceMan.GetDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, isRenderTargetOrDepthStencil ? &clearValue : nullptr, IID_PPV_ARGS(&id));
	TexDesc texDesc;
	texDesc.size = size;
	if (image) {
		afWriteTexture(id, texDesc, image);
	}
	return id;
}

SRVID afCreateTexture2D(AFDTFormat format, const struct TexDesc& desc, int mipCount, const AFTexSubresourceData datas[])
{
	bool isDepthStencil = format == AFDT_DEPTH || format == AFDT_DEPTH_STENCIL;
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = mipCount;
	textureDesc.Format = format;
	textureDesc.Width = desc.size.x;
	textureDesc.Height = desc.size.y;
	textureDesc.Flags = isDepthStencil ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = desc.arraySize;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	SRVID id;
	D3D12_CLEAR_VALUE clearValue = { format };
	if (isDepthStencil) {
		clearValue.DepthStencil.Depth = 1.0f;
	}
	HRESULT hr = deviceMan.GetDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, isDepthStencil ? &clearValue : nullptr, IID_PPV_ARGS(&id));
	afWriteTexture(id, desc, mipCount, datas);
	return id;
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

ComPtr<ID3D12PipelineState> afCreatePSO(const char *shaderName, const InputElement elements[], int numElements, BlendMode blendMode, DepthStencilMode depthStencilMode, CullMode cullMode, ComPtr<ID3D12RootSignature> rootSignature, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology)
{
	ComPtr<ID3DBlob> vertexShader = afCompileHLSL(shaderName, "VSMain", "vs_5_0");
	ComPtr<ID3DBlob> pixelShader = afCompileHLSL(shaderName, "PSMain", "ps_5_0");

	static D3D12_RENDER_TARGET_BLEND_DESC solid = {
		FALSE, FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
	};
	static D3D12_RENDER_TARGET_BLEND_DESC alphaBlend = {
		TRUE, FALSE,
		D3D12_BLEND_SRC_ALPHA, D3D12_BLEND_INV_SRC_ALPHA, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	switch (blendMode) {
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
	psoDesc.DSVFormat = AFDT_DEPTH_STENCIL;
	psoDesc.SampleDesc.Count = 1;
	ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = deviceMan.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	assert(hr == S_OK);
	return pso;
}

class CDescriptorCBV : public D3D12_DESCRIPTOR_RANGE {
public:
	CDescriptorCBV(int shaderRegister) {
		RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		NumDescriptors = 1;
		BaseShaderRegister = shaderRegister;
		RegisterSpace = 0;
		OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}
};

class CDescriptorSRV : public D3D12_DESCRIPTOR_RANGE {
public:
	CDescriptorSRV(int shaderRegister) {
		RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		NumDescriptors = 1;
		BaseShaderRegister = shaderRegister;
		RegisterSpace = 0;
		OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}
};

ComPtr<ID3D12RootSignature> afCreateRootSignature(DescriptorLayout descriptorLayout, int numSamplers, const SamplerType samplers[])
{
	ComPtr<ID3D12RootSignature> rs;
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[16] = {};
	assert(numSamplers < (int)dimof(staticSamplers));
	for (int i = 0; i < numSamplers; i++) {
		D3D12_STATIC_SAMPLER_DESC& desc = staticSamplers[i];
		switch (samplers[i] >> 1) {
		case 2:	// mipmap
			desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case 1:	// linear
			desc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case 0:	// point
			desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		}
		desc.AddressU = desc.AddressV = (samplers[i] & 0x01) ? D3D12_TEXTURE_ADDRESS_MODE_CLAMP : D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		desc.MipLODBias = 0;
		desc.MaxAnisotropy = 1;
		desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		desc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		desc.MinLOD = 0;
		desc.MaxLOD = D3D12_FLOAT32_MAX;
		desc.ShaderRegister = i;
		desc.RegisterSpace = 0;
		desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}

	UINT numRootParameters = 0;
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	static D3D12_DESCRIPTOR_RANGE descriptors[] =
	{
		CDescriptorSRV(0),
	};
	switch (descriptorLayout) {
	case AFDL_NONE:
		numRootParameters = 0;
		break;
	case AFDL_CBV0:
		{
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			numRootParameters = 1;
			break;
		}
	case AFDL_CBV0_SRV0:
		{
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameters[1].DescriptorTable.NumDescriptorRanges = _countof(descriptors);
			rootParameters[1].DescriptorTable.pDescriptorRanges = descriptors;
			numRootParameters = 2;
			break;
		}
	case AFDL_SRV0:
		{
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(descriptors);
			rootParameters[0].DescriptorTable.pDescriptorRanges = descriptors;
			numRootParameters = 1;
			break;
		}
	case AFDL_CBV012_SRV0:
		{
			for (int i = 0; i < 3; i++) {
				rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
				rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				rootParameters[i].Descriptor.ShaderRegister = i;
			}
			rootParameters[3].DescriptorTable.NumDescriptorRanges = _countof(descriptors);
			rootParameters[3].DescriptorTable.pDescriptorRanges = descriptors;
			numRootParameters = 4;
			break;
		}
	}

	D3D12_ROOT_SIGNATURE_DESC rsDesc = { numRootParameters, rootParameters, (UINT)numSamplers, staticSamplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
	HRESULT hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	assert(hr == S_OK);
	hr = deviceMan.GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rs));
	assert(hr == S_OK);
	return rs;
}

ComPtr<ID3D12DescriptorHeap> afCreateDescriptorHeap(int numSrvs, SRVID srvs[])
{
	ComPtr<ID3D12DescriptorHeap> heap;
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = numSrvs;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HRESULT hr = deviceMan.GetDevice()->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&heap));
	assert(hr == S_OK);
	for (int i = 0; i < numSrvs; i++) {
		D3D12_RESOURCE_DESC desc = srvs[i]->GetDesc();
		auto ptr = heap->GetCPUDescriptorHandleForHeapStart();
		ptr.ptr += deviceMan.GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * i;
		if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = srvs[i]->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (UINT)desc.Width;
			assert((cbvDesc.SizeInBytes & 0xff) == 0);
			deviceMan.GetDevice()->CreateConstantBufferView(&cbvDesc, ptr);
		} else {
			deviceMan.GetDevice()->CreateShaderResourceView(srvs[i].Get(), nullptr, ptr);
		}
	}

	return heap;
}

void afWaitFenceValue(ComPtr<ID3D12Fence> fence, UINT64 value)
{
	if (fence->GetCompletedValue() >= value) {
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
	VBOID vbo = afCreateDynamicVertexBuffer(size, buf);
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
	VBOID vbo = afCreateDynamicVertexBuffer(size, buf);
	afSetVertexBuffer(vbo, stride);
	deviceMan.AddIntermediateCommandlistDependentResource(vbo);
}

void AFRenderTarget::InitForDefaultRenderTarget()
{
	asDefault = true;
}

void AFRenderTarget::Init(IVec2 size, AFDTFormat colorFormat, AFDTFormat depthStencilFormat)
{
	texSize = size;
	renderTarget = afCreateDynamicTexture(colorFormat, size, nullptr, true);
	afSetTextureName(renderTarget, __FUNCTION__);
	ID3D12GraphicsCommandList* commandList = deviceMan.GetCommandList();
	const D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1 };
	deviceMan.GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	deviceMan.GetDevice()->CreateRenderTargetView(renderTarget.Get(), nullptr, rtvHandle);
}

void AFRenderTarget::Destroy()
{
	rtvHeap.Reset();
	renderTarget.Reset();
}

void AFRenderTarget::BeginRenderToThis()
{
	if (asDefault) {
		deviceMan.SetRenderTarget();
		return;
	}

	D3D12_VIEWPORT vp = { 0.f, 0.f, (float)texSize.x, (float)texSize.y, 0.f, 1.f };
	D3D12_RECT rc = { 0, 0, (LONG)texSize.x, (LONG)texSize.y };
	ID3D12GraphicsCommandList* commandList = deviceMan.GetCommandList();
	commandList->RSSetViewports(1, &vp);
	commandList->RSSetScissorRects(1, &rc);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = deviceMan.GetDepthStencilView();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();

	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
}

FakeVAO::FakeVAO(int numBuffers, VBOID const vbos_[], const int strides_[], const UINT offsets_[], IBOID ibo_)
{
	ibo = ibo_;
	vbos.resize(numBuffers);
	strides.resize(numBuffers);
	offsets.resize(numBuffers);
	for (int i = 0; i < numBuffers; i++) {
		vbos[i] = vbos_[i];
		strides[i] = (UINT)strides_[i];
		offsets[i] = offsets_ ? offsets_[i] : 0;
	}
}

void FakeVAO::Apply()
{
	afSetVertexBuffers((int)vbos.size(), vbos.data(), strides.data());
	if (ibo) {
		afSetIndexBuffer(ibo);
	}
}

void afBindCbvs(AFCbvBindToken cbvs[], int nCbvs)
{
	for (int i = 0; i < nCbvs; i++) {
		AFCbvBindToken& cbv = cbvs[i];
		if (cbv.top >= 0) {
			deviceMan.GetCommandList()->SetGraphicsRootConstantBufferView(i, deviceMan.GetConstantBufferGPUAddress(cbv.top));
		} else if (cbv.ubo) {
			afBindBufferToBindingPoint(cbv.ubo, i);
		} else {
			assert(0);
		}
	}
}

void afBindBufferToBindingPoint(const void* buf, int size, int rootParameterIndex)
{
	int cbTop = deviceMan.AssignConstantBuffer(buf, size);
	deviceMan.GetCommandList()->SetGraphicsRootConstantBufferView(rootParameterIndex, deviceMan.GetConstantBufferGPUAddress(cbTop));
}

void afBindBufferToBindingPoint(UBOID ubo, int rootParameterIndex)
{
	deviceMan.GetCommandList()->SetGraphicsRootConstantBufferView(rootParameterIndex, ubo->GetGPUVirtualAddress());
}

VAOID afCreateVAO(const InputElement elements[], int numElements, int numBuffers, VBOID const vertexBufferIds[], const int strides[], IBOID ibo)
{
	(void)elements;
	(void)numElements;
	VAOID p(new FakeVAO(numBuffers, vertexBufferIds, strides, nullptr, ibo));
	return p;
}

void afBindVAO(const VAOID& vao)
{
	if (vao) {
		vao->Apply();
	}
}

#endif
