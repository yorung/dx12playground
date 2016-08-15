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

struct TexDesc {
	IVec2 size;
	int arraySize = 1;
	bool isCubeMap = false;
};

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

enum SamplerType {
	AFST_POINT_WRAP,
	AFST_POINT_CLAMP,
	AFST_LINEAR_WRAP,
	AFST_LINEAR_CLAMP,
	AFST_MIPMAP_WRAP,
	AFST_MIPMAP_CLAMP,
	AFST_MAX
};

enum DescriptorLayout
{
	AFDL_NONE,
	AFDL_CBV0,
	AFDL_SRV0,
	AFDL_CBV0_SRV0,
	AFDL_CBV012_SRV0,
	AFDL_CBV0_SRV012345,
	AFDL_SRV0123456,
};

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
