class Triangle
{
	VBOID vbo;
	IBOID ibo;
	UBOID ubo;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12DescriptorHeap> heap;
public:
	void Create();
	void Update();
	void Draw();
	void Destroy();
};

extern Triangle triangle;
