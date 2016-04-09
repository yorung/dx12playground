#include "stdafx.h"

ComPtr<ID3DBlob> afCompileShader(const char* name, const char* entryPoint, const char* target)
{
	char path[MAX_PATH];
	sprintf_s(path, sizeof(path), "hlsl/%s.hlsl", name);
#ifdef _DEBUG
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#endif
	ComPtr<ID3DBlob> blob, err;
	WCHAR wname[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, path, -1, wname, dimof(wname));
	HRESULT hr = D3DCompileFromFile(wname, nullptr, nullptr, entryPoint, target, flags, 0, &blob, &err);
	if (err) {
		MessageBoxA(nullptr, (const char*)err->GetBufferPointer(), name, MB_OK | MB_ICONERROR);
	}
	return blob;
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

void afSetVertexBuffer(VBOID id, int stride)
{
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();
	D3D12_RESOURCE_DESC desc = id->GetDesc();
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = { id->GetGPUVirtualAddress(), (UINT)desc.Width, (UINT)stride };
	list->IASetVertexBuffers(0, 1, &vertexBufferView);
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
	id->Map(0, &readRange, &p);
	memcpy(p, buf, size);
	D3D12_RANGE wroteRange = {0, (SIZE_T)size};
	id->Unmap(0, &wroteRange);
}

ComPtr<ID3D12Resource> afCreateBuffer(int size, const void* buf)
{
	D3D12_HEAP_PROPERTIES prop = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	D3D12_RESOURCE_DESC desc = { D3D12_RESOURCE_DIMENSION_BUFFER, 0, (UINT64)size, 1, 1, 1, DXGI_FORMAT_UNKNOWN, { 1, 0 }, D3D12_TEXTURE_LAYOUT_ROW_MAJOR, D3D12_RESOURCE_FLAG_NONE };
	UBOID o;
	deviceMan.GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&o));
	if (buf) {
		afWriteBuffer(o, buf, size);
	}
	return o;
}

VBOID afCreateVertexBuffer(int size, const void* buf)
{
	return afCreateBuffer(size, buf);
}

IBOID afCreateIndexBuffer(const AFIndex* indi, int numIndi)
{
	return afCreateBuffer(numIndi * sizeof(AFIndex), indi);
}

UBOID afCreateUBO(int size)
{
	return afCreateBuffer((size + 0xff) & ~0xff);
}

void afWriteTexture(SRVID id, const TexDesc& desc, const void* buf)
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
	UINT numRow;
	UINT64 uploadSize, rowSizeInBytes;
	D3D12_RESOURCE_DESC destDesc = id->GetDesc();
	deviceMan.GetDevice()->GetCopyableFootprints(&destDesc, 0, 1, 0, &footprint, &numRow, &rowSizeInBytes, &uploadSize);
	ComPtr<ID3D12Resource> uploadBuf = afCreateBuffer((int)uploadSize, buf);
	D3D12_TEXTURE_COPY_LOCATION uploadBufLocation = { uploadBuf.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, footprint }, nativeBufLocation = { id.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, 0 };
	ID3D12GraphicsCommandList* list = deviceMan.GetCommandList();

	D3D12_RESOURCE_BARRIER transition1 = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, { id.Get(), 0, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST } };
	list->ResourceBarrier(1, &transition1);
	list->CopyTextureRegion(&nativeBufLocation, 0, 0, 0, &uploadBufLocation, nullptr);
	D3D12_RESOURCE_BARRIER transition2 = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION, D3D12_RESOURCE_BARRIER_FLAG_NONE, { id.Get(), 0, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ } };
	list->ResourceBarrier(1, &transition2);
	deviceMan.Flush();
}

SRVID afCreateTexture2D(AFDTFormat format, const IVec2& size, void *image)
{
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = size.x;
	textureDesc.Height = size.y;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	SRVID id;
	D3D12_HEAP_PROPERTIES prop = { D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
	HRESULT hr = deviceMan.GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&id));
	TexDesc texDesc;
	texDesc.size = size;
	afWriteTexture(id, texDesc, image);
	return id;
}

SRVID afCreateTexture2D(AFDTFormat format, const struct TexDesc& desc, int mipCount, const AFTexSubresourceData datas[])
{
	// TODO:
	return SRVID();
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

ComPtr<ID3D12PipelineState> afCreatePSO(const char *shaderName, const InputElement elements[], int numElements, BlendMode blendMode, DepthStencilMode depthStencilMode, CullMode cullMode, ComPtr<ID3D12RootSignature> rootSignature)
{
	ComPtr<ID3DBlob> vertexShader = afCompileShader(shaderName, "VSMain", "vs_5_0");
	ComPtr<ID3DBlob> pixelShader = afCompileShader(shaderName, "PSMain", "ps_5_0");

	D3D12_BLEND_DESC bd = { FALSE, FALSE,{
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL },
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { elements, (UINT)numElements };
	psoDesc.pRootSignature = rootSignature.Get();
	psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
	psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
	psoDesc.RasterizerState = { D3D12_FILL_MODE_SOLID, cullMode == CM_CCW ? D3D12_CULL_MODE_BACK : cullMode == CM_CW ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_NONE };
	psoDesc.BlendState = bd;
	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	ComPtr<ID3D12PipelineState> pso;
	HRESULT hr = deviceMan.GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
	assert(hr == S_OK);
	return pso;
}

ComPtr<ID3D12RootSignature> afCreateRootSignature(int numDescriptors, Descriptor descriptors[], int numSamplers, Sampler samplers[])
{
	ComPtr<ID3D12RootSignature> rs;
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;

	D3D12_ROOT_PARAMETER rootParameter = {};
	rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	rootParameter.DescriptorTable.NumDescriptorRanges = numDescriptors;
	rootParameter.DescriptorTable.pDescriptorRanges = descriptors;

	D3D12_ROOT_SIGNATURE_DESC rsDesc = { (UINT)(numDescriptors ? 1 : 0), &rootParameter, (UINT)numSamplers, samplers, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT };
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
