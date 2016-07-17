#define dimof(x) (sizeof(x) / sizeof(x[0]))

void *LoadFile(const char *fileName, int* size = nullptr);
bool SaveFile(const char *fileName, const uint8_t* buf, int size);
double GetTime();
float Random();
void GoMyDir();
void Toast(const char *text);
void PlayBgm(const char *fileName);
const char* StrMessageBox(const char* txt, const char* type);
void ClearMenu();
void AddMenu(const char *name, const char *cmd);
void PostCommand(const char* cmdString);
bool LoadImageViaGdiPlus(const char* name, IVec2& size, std::vector<uint32_t>& col);
SRVID LoadTextureViaOS(const char* name, IVec2& size);

template <class T> inline void SAFE_DELETE(T& p)
{
	delete p;
	p = nullptr;
}

template <class T> inline void SAFE_RELEASE(T& p)
{
	if (p) {
		p->Release();
		p = nullptr;
	}
}

IBOID afCreateTiledPlaneIBO(int numTiles, int* numIndies = nullptr);
VBOID afCreateTiledPlaneVBO(int numTiles);
IBOID afCreateQuadListIndexBuffer(int numQuads);

struct TexDesc {
	IVec2 size;
	int arraySize = 1;
	bool isCubeMap = false;
};

SRVID afLoadTexture(const char* name, TexDesc& desc);

struct CharSignature {
	wchar_t code;
	int fontSize;
	inline int GetOrder() const { return (code << 8) | fontSize; }
	bool operator < (const CharSignature& r) const { return GetOrder() < r.GetOrder(); }
	bool operator == (const CharSignature& r) const { return GetOrder() == r.GetOrder(); }
};
struct CharDesc {
	Vec2 srcWidth;
	Vec2 distDelta;
	float step;
};
void MakeFontBitmap(const char* fontName, const CharSignature& code, class DIB& dib, CharDesc& desc);

void afVerify(bool ok);
