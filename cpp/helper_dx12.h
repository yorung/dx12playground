#include "AFGraphicsDefinitions.inl"

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

typedef ComPtr<ID3D12Resource> AFBufferResource;
typedef ComPtr<ID3D12Resource> SRVID;
inline void afSafeDeleteBuffer(ComPtr<ID3D12Resource>& p) { p.Reset(); }
inline void afSafeDeleteTexture(SRVID& p) { p.Reset(); }

void afSetPipeline(ComPtr<ID3D12PipelineState> ps, ComPtr<ID3D12RootSignature> rs);
void afSetDescriptorHeap(ComPtr<ID3D12DescriptorHeap> heap);
void afSetVertexBuffer(AFBufferResource id, int stride);
void afSetVertexBuffers(int numIds, AFBufferResource ids[], int strides[]);
void afSetIndexBuffer(AFBufferResource id);
void afWriteBuffer(const AFBufferResource id, const void* buf, int size);
ComPtr<ID3D12Resource> afCreateBuffer(int size, const void* buf = nullptr);
AFBufferResource afCreateVertexBuffer(int size, const void* buf = nullptr);
AFBufferResource afCreateIndexBuffer(const AFIndex* indi, int numIndi);
ComPtr<ID3D12Resource> afCreateDynamicVertexBuffer(int size, const void* buf = nullptr);
AFBufferResource afCreateUBO(int size);

ComPtr<ID3D12PipelineState> afCreatePSO(const char *shaderName, const InputElement elements[], int numElements, BlendMode blendMode, DepthStencilMode depthStencilMode, CullMode cullMode, ComPtr<ID3D12RootSignature>& rootSignature, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology);

void afDrawIndexed(PrimitiveTopology pt, int numIndices, int start = 0, int instanceCount = 1);
void afDraw(PrimitiveTopology pt, int numVertices, int start = 0, int instanceCount = 1);

struct AFTexSubresourceData
{
	const void* ptr;
	uint32_t pitch;
	uint32_t pitchSlice;
};

ComPtr<ID3D12Resource> afCreateDynamicTexture(AFFormat format, const IVec2& size, uint32_t flags = AFTF_CPU_WRITE | AFTF_SRV);
SRVID afCreateTexture2D(AFFormat format, const IVec2& size, void *image = nullptr, bool isRenderTargetOrDepthStencil = false);
SRVID afCreateTexture2D(AFFormat format, const struct TexDesc& desc, int mipCount, const AFTexSubresourceData datas[]);
void afSetTextureName(SRVID tex, const char* name);

void afWriteTexture(SRVID srv, const TexDesc& desc, const void* buf);
void afWriteTexture(SRVID id, const TexDesc& desc, int mipCount, const AFTexSubresourceData datas[]);

SRVID LoadTextureViaOS(const char* name, IVec2& size);
AFBufferResource afCreateTiledPlaneIBO(int numTiles, int* numIndies = nullptr);
SRVID afLoadTexture(const char* name, TexDesc& desc);
AFBufferResource afCreateTiledPlaneVBO(int numTiles);
AFBufferResource afCreateQuadListIndexBuffer(int numQuads);

ComPtr<ID3D12DescriptorHeap> afCreateDescriptorHeap(int numSrvs, SRVID srvs[]);
void afWaitFenceValue(ComPtr<ID3D12Fence> fence, UINT64 value);
IVec2 afGetTextureSize(SRVID tex);

void afSetVertexBufferFromSystemMemory(const void* buf, int size, int stride);

class AFRenderStates {
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
public:
	bool IsReady() { return !!pipelineState; }
	void Create(const char* shaderName, int numInputElements, const InputElement* inputElements, BlendMode blendMode_, DepthStencilMode depthStencilMode_, CullMode cullMode_, int numSamplerTypes_ = 0, const SamplerType samplerTypes_[] = nullptr, D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE)
	{
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
	AFBufferResource ibo;
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
	AFBufferResource ubo;
	int top = -1;
	int size = 0;
	void Create(AFBufferResource ubo_)
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
	ComPtr<ID3D12Resource> renderTarget;
	D3D12_RESOURCE_STATES currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	bool asDefault = false;
public:
	~AFRenderTarget() { Destroy(); }
	void InitForDefaultRenderTarget();
	void Init(IVec2 size, AFFormat colorFormat, AFFormat depthStencilFormat = AFF_INVALID);
	void Destroy();
	void BeginRenderToThis();
	ComPtr<ID3D12Resource> GetTexture();
};

class FakeVAO
{
	std::vector<AFBufferResource> vbos;
	std::vector<UINT> offsets;
	std::vector<int> strides;
	ComPtr<ID3D12Resource> ibo;
public:
	FakeVAO(int numBuffers, AFBufferResource const buffers[], const int strides[], const UINT offsets[], AFBufferResource ibo);
	void Apply();
};

typedef std::unique_ptr<FakeVAO> VAOID;
VAOID afCreateVAO(const InputElement elements[], int numElements, int numBuffers, AFBufferResource const vertexBufferIds[], const int strides[], AFBufferResource ibo);
void afBindVAO(const VAOID& vao);
inline void afSafeDeleteVAO(VAOID& p) { p.reset(); }

void afBindBufferToBindingPoint(const void* buf, int size, int rootParameterIndex);
void afBindBufferToBindingPoint(AFBufferResource ubo, int rootParameterIndex);
void afBindTextureToBindingPoint(SRVID srv, int rootParameterIndex);
#define afBindCubeMapToBindingPoint afBindTextureToBindingPoint
