class SkyMan
{
	SRVID texId;
	TexDesc texDesc;
	AFRenderStates renderStates;
public:
	~SkyMan();
	void Create(const char *texFileName, const char* shader);
	void Draw();
	void Destroy();
};

extern SkyMan skyMan;
