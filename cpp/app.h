class App {
	VBOID vbo;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12PipelineState> pipelineState;
public:
	App();
	void Init();
	void Update();
	void Draw();
	void Destroy();
};

extern App app;

extern std::string g_type;
