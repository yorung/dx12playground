ComPtr<ID3DBlob> afCompileShader(const char* name, const char* entryPoint, const char* target);

#define SF_R32G32_FLOAT DXGI_FORMAT_R32G32_FLOAT
#define SF_R32G32B32_FLOAT DXGI_FORMAT_R32G32B32_FLOAT
#define SF_R32G32B32A32_FLOAT DXGI_FORMAT_R32G32B32A32_FLOAT
#define SF_R8G8B8A8_UNORM DXGI_FORMAT_R8G8B8A8_UNORM

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

typedef unsigned short AFIndex;

#define AFIndexTypeToDevice DXGI_FORMAT_R16_UINT
typedef ComPtr<ID3D12Resource> IBOID;
typedef ComPtr<ID3D12Resource> VBOID;
typedef ComPtr<ID3D12Resource> UBOID;
typedef ComPtr<ID3D12Resource> SRVID;
inline void afSafeDeleteBuffer(ComPtr<ID3D12Resource>& p) { p.Reset(); }
inline void afSafeDeleteTexture(SRVID& p) { p.Reset(); }

void afSetPipeline(ComPtr<ID3D12PipelineState> ps, ComPtr<ID3D12RootSignature> rs);
void afSetDescriptorHeap(ComPtr<ID3D12DescriptorHeap> heap);
void afSetVertexBuffer(VBOID id, int stride);
void afSetIndexBuffer(IBOID id);
void afWriteBuffer(const IBOID id, const void* buf, int size);
ComPtr<ID3D12Resource> afCreateBuffer(int size, const void* buf = nullptr);
VBOID afCreateVertexBuffer(int size, const void* buf = nullptr);
IBOID afCreateIndexBuffer(const AFIndex* indi, int numIndi);
ComPtr<ID3D12Resource> afCreateDynamicVertexBuffer(int size, const void* buf = nullptr);
UBOID afCreateUBO(int size);

enum CullMode {
	CM_DISABLE,
	CM_CW,
	CM_CCW,
};
enum BlendMode {
	BM_NONE,
	BM_ALPHA,
};
enum DepthStencilMode {
	DSM_DISABLE,
	DSM_DEPTH_ENABLE,
	DSM_DEPTH_CLOSEREQUAL_READONLY,
};

#define SamplerWrap D3D12_TEXTURE_ADDRESS_MODE
#define SW_REPEAT D3D12_TEXTURE_ADDRESS_MODE_WRAP
#define SW_CLAMP D3D12_TEXTURE_ADDRESS_MODE_CLAMP

#define SamplerFilter D3D12_FILTER
#define SF_POINT D3D12_FILTER_MIN_MAG_MIP_POINT
#define SF_LINEAR D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT
#define SF_MIPMAP D3D12_FILTER_MIN_MAG_MIP_LINEAR

class CSampler : public D3D12_STATIC_SAMPLER_DESC
{
public:
	CSampler(int shaderRegister, SamplerFilter samplerFilter, SamplerWrap wrap)
	{
		Filter = samplerFilter;
		AddressU = wrap;
		AddressV = wrap;
		AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		MipLODBias = 0;
		MaxAnisotropy = 1;
		ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		MinLOD = 0;
		MaxLOD = D3D12_FLOAT32_MAX;
		ShaderRegister = shaderRegister;
		RegisterSpace = 0;
		ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	}
};

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

ComPtr<ID3D12PipelineState> afCreatePSO(const char *shaderName, const InputElement elements[], int numElements, BlendMode blendMode, DepthStencilMode depthStencilMode, CullMode cullMode, ComPtr<ID3D12RootSignature> rootSignature);
ComPtr<ID3D12RootSignature> afCreateRootSignature(int numDescriptors, D3D12_DESCRIPTOR_RANGE descriptors[], int numSamplers, D3D12_STATIC_SAMPLER_DESC samplers[]);

#define PrimitiveTopology D3D_PRIMITIVE_TOPOLOGY
#define PT_TRIANGLESTRIP D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
#define PT_TRIANGLELIST D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
#define PT_LINELIST D3D_PRIMITIVE_TOPOLOGY_LINELIST

void afDrawIndexed(PrimitiveTopology pt, int numIndices, int start = 0, int instanceCount = 1);
void afDraw(PrimitiveTopology pt, int numVertices, int start = 0, int instanceCount = 1);

typedef D3D12_SUBRESOURCE_DATA AFTexSubresourceData;
typedef DXGI_FORMAT AFDTFormat;
#define AFDT_INVALID DXGI_FORMAT_UNKNOWN
#define AFDT_R8G8B8A8_UNORM DXGI_FORMAT_R8G8B8A8_UNORM
#define AFDT_R5G6B5_UINT DXGI_FORMAT_B5G6R5_UNORM
#define AFDT_R32G32B32A32_FLOAT DXGI_FORMAT_R32G32B32A32_FLOAT
#define AFDT_R16G16B16A16_FLOAT DXGI_FORMAT_R16G16B16A16_FLOAT
#define AFDT_DEPTH DXGI_FORMAT_D24_UNORM_S8_UINT
#define AFDT_DEPTH_STENCIL DXGI_FORMAT_D24_UNORM_S8_UINT
#define AFDT_BC1_UNORM DXGI_FORMAT_BC1_UNORM
#define AFDT_BC2_UNORM DXGI_FORMAT_BC2_UNORM
#define AFDT_BC3_UNORM DXGI_FORMAT_BC3_UNORM

SRVID afCreateTexture2D(AFDTFormat format, const IVec2& size, void *image = nullptr);
SRVID afCreateTexture2D(AFDTFormat format, const struct TexDesc& desc, int mipCount, const AFTexSubresourceData datas[]);
void afWriteTexture(SRVID srv, const TexDesc& desc, const void* buf);
#define afCreateDynamicTexture afCreateTexture2D

ComPtr<ID3D12DescriptorHeap> afCreateDescriptorHeap(int numSrvs, SRVID srvs[]);
