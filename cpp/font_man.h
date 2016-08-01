class FontMan
{
	struct CharCache {
		Vec2 srcPos;
		CharDesc desc;
	};
	struct CharSprite {
		Vec2 pos;
		CharSignature signature;
	};
	typedef std::map<CharSignature, CharCache> Caches;
	Caches caches;
	int curX, curY, curLineMaxH;
	SRVID texture;
	DIB texSrc;

	static const int SPRITE_MAX = 4096;
	static const int SPRITE_VERTS = SPRITE_MAX * 4;
	CharSprite charSprites[SPRITE_MAX];
	int numSprites;
	AFRenderStates renderStates;
	AFDynamicQuadListVertexBuffer quadListVertexBuffer;
	bool dirty = false;
	bool Build(const CharSignature& signature);
	bool Cache(const CharSignature& code);
	void DrawChar(Vec2& pos, const CharSignature& sig);
	void ClearCache();
public:
	FontMan();
	~FontMan();
	bool Init();
	void Destroy();
	void FlushToTexture();
	void DrawString(Vec2 pos, int fontSize, const wchar_t *text);
	void DrawString(Vec2 pos, int fontSize, const char *text);
	void Render();
	Vec2 MeasureString(int fontSize, const char *text);
};

extern FontMan fontMan;
