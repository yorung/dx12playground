class Triangle
{
	VBOID vbo;
	IBOID ibo;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
public:
	void Create();
	void Draw();
	void Destroy();
};

extern Triangle triangle;
