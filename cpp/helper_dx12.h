typedef D3D12_INPUT_ELEMENT_DESC InputElement;

class CInputElement : public InputElement {
public:
	CInputElement(const char* name, DXGI_FORMAT format, int offset, int inputSlot = 0) {
		SemanticName = name;
		SemanticIndex = 0;
		Format = format;
		InputSlot = inputSlot;
		AlignedByteOffset = offset;
		InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		InstanceDataStepRate = 0;
	}
};

typedef ComPtr<ID3D12Resource> IBOID;
typedef ComPtr<ID3D12Resource> VBOID;
typedef ComPtr<ID3D12Resource> UBOID;
typedef ComPtr<ID3D12Resource> SRVID;
inline void afSafeDeleteBuffer(ComPtr<ID3D12Resource>& p) { p.Reset(); }
inline void afSafeDeleteTexture(SRVID& p) { p.Reset(); }

void afSetPipeline(ComPtr<ID3D12PipelineState> ps, ComPtr<ID3D12RootSignature> rs);
void afSetDescriptorHeap(ComPtr<ID3D12DescriptorHeap> heap);
void afSetVertexBuffer(VBOID id, int stride);
void afSetVertexBuffers(int numIds, VBOID ids[], int strides[]);
void afSetIndexBuffer(IBOID id);
void afWriteBuffer(const IBOID id, const void* buf, int size);
ComPtr<ID3D12Resource> afCreateBuffer(int size, const void* buf = nullptr);
VBOID afCreateVertexBuffer(int size, const void* buf = nullptr);
IBOID afCreateIndexBuffer(const AFIndex* indi, int numIndi);
ComPtr<ID3D12Resource> afCreateDynamicVertexBuffer(int size, const void* buf = nullptr);
UBOID afCreateUBO(int size);

ComPtr<ID3D12PipelineState> afCreatePSO(const char *shaderName, const InputElement elements[], int numElements, BlendMode blendMode, DepthStencilMode depthStencilMode, CullMode cullMode, ComPtr<ID3D12RootSignature> rootSignature, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology);
ComPtr<ID3D12RootSignature> afCreateRootSignature(DescriptorLayout descriptorLayout, int numSamplers, const SamplerType samplers[]);

void afDrawIndexed(PrimitiveTopology pt, int numIndices, int start = 0, int instanceCount = 1);
void afDraw(PrimitiveTopology pt, int numVertices, int start = 0, int instanceCount = 1);

struct AFTexSubresourceData
{
	const void* ptr;
	uint32_t pitch;
	uint32_t pitchSlice;
};

SRVID afCreateTexture2D(AFDTFormat format, const IVec2& size, void *image = nullptr, bool isRenderTargetOrDepthStencil = false);
SRVID afCreateTexture2D(AFDTFormat format, const struct TexDesc& desc, int mipCount, const AFTexSubresourceData datas[]);
void afSetTextureName(SRVID tex, const char* name);

void afWriteTexture(SRVID srv, const TexDesc& desc, const void* buf);
void afWriteTexture(SRVID id, const TexDesc& desc, int mipCount, const AFTexSubresourceData datas[]);
#define afCreateDynamicTexture afCreateTexture2D

SRVID LoadTextureViaOS(const char* name, IVec2& size);
IBOID afCreateTiledPlaneIBO(int numTiles, int* numIndies = nullptr);
SRVID afLoadTexture(const char* name, TexDesc& desc);
VBOID afCreateTiledPlaneVBO(int numTiles);
IBOID afCreateQuadListIndexBuffer(int numQuads);

ComPtr<ID3D12DescriptorHeap> afCreateDescriptorHeap(int numSrvs, SRVID srvs[]);
void afWaitFenceValue(ComPtr<ID3D12Fence> fence, UINT64 value);
IVec2 afGetTextureSize(SRVID tex);

void afSetVertexBufferFromSystemMemory(const void* buf, int size, int stride);

class AFRenderStates {
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
public:
	bool IsReady() { return !!pipelineState; }
	void Create(DescriptorLayout descriptorLayout, const char* shaderName, int numInputElements, const InputElement* inputElements, BlendMode blendMode_, DepthStencilMode depthStencilMode_, CullMode cullMode_, int numSamplerTypes_ = 0, const SamplerType samplerTypes_[] = nullptr, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
	{
		rootSignature = afCreateRootSignature(descriptorLayout, numSamplerTypes_, samplerTypes_);
		pipelineState = afCreatePSO(shaderName, inputElements, numInputElements, blendMode_, depthStencilMode_, cullMode_, rootSignature, primitiveTopology);
	}
	void Apply() const
	{
		afSetPipeline(pipelineState, rootSignature);
	}
	void Destroy()
	{
		pipelineState.Reset();
		rootSignature.Reset();
	}
};

class AFDynamicQuadListVertexBuffer {
	IBOID ibo;
	UINT stride;
	int vertexBufferSize;
public:
	~AFDynamicQuadListVertexBuffer() { Destroy(); }
	void Create(const InputElement*, int, int vertexSize_, int nQuad);
	void Apply()
	{
		afSetIndexBuffer(ibo);
	}
	void Write(const void* buf, int size);
	void Destroy()
	{
		afSafeDeleteBuffer(ibo);
	}
};

class AFCbvBindToken {
public:
	UBOID ubo;
	int top = -1;
	int size = 0;
	void Create(UBOID ubo_)
	{
		ubo = ubo_;
	}
	void Create(const void* buf, int size_)
	{
		top = deviceMan.AssignConstantBuffer(buf, size_);
		size = size_;
	}
};

void afBindCbvs(AFCbvBindToken cbvs[], int nCbvs);

class AFRenderTarget
{
	IVec2 texSize;
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12Resource> renderTarget;
	bool asDefault = false;
public:
	~AFRenderTarget() { Destroy(); }
	void InitForDefaultRenderTarget();
	void Init(IVec2 size, AFDTFormat colorFormat, AFDTFormat depthStencilFormat = AFDT_INVALID);
	void Destroy();
	void BeginRenderToThis();
	ComPtr<ID3D12Resource> GetTexture() { return renderTarget; }
};

class FakeVAO
{
	std::vector<VBOID> vbos;
	std::vector<UINT> offsets;
	std::vector<int> strides;
	ComPtr<ID3D12Resource> ibo;
public:
	FakeVAO(int numBuffers, VBOID const buffers[], const int strides[], const UINT offsets[], IBOID ibo);
	void Apply();
};

typedef std::unique_ptr<FakeVAO> VAOID;
VAOID afCreateVAO(const InputElement elements[], int numElements, int numBuffers, VBOID const vertexBufferIds[], const int strides[], IBOID ibo);
void afBindVAO(const VAOID& vao);
inline void afSafeDeleteVAO(VAOID& p) { p.reset(); }

void afBindBufferToBindingPoint(const void* buf, int size, int rootParameterIndex);
void afBindBufferToBindingPoint(UBOID ubo, int rootParameterIndex);
void afBindTextureToBindingPoint(SRVID srv, int rootParameterIndex);
#define afBindCubeMapToBindingPoint afBindTextureToBindingPoint

constexpr int afGetTRegisterBindingPoint(DescriptorLayout layout)
{
	return (layout == AFDL_CBV0_SRV0) ? 1 : (layout == AFDL_CBV012_SRV0) ? 3 : 0;
}
