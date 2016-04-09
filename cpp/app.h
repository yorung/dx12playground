class App {
	VBOID vbo;
	UBOID ubo;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12DescriptorHeap> heap;
public:
	App();
	void Init();
	void Update();
	void Draw();
	void Destroy();
};

extern App app;

extern std::string g_type;
