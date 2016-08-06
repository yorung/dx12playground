class App {
	AFRenderTarget rt;
	AFRenderStates rsPostProcess;
public:
	App();
	void Init();
	void Update();
	void Draw();
	void Destroy();
};

extern App app;

extern std::string g_type;
