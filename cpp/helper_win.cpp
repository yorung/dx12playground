#include "stdafx.h"

#ifdef _MSC_VER

bool SaveFile(const char *fileName, const uint8_t* buf, int size)
{
	bool result = false;
	FILE *f = nullptr;

	if (fopen_s(&f, fileName, "wb")) {
		return false;
	}
	if (!fwrite(buf, size, 1, f)) {
		goto DONE;
	}
	result = !fclose(f);
	f = nullptr;
DONE:
	if (f) {
		fclose(f);
	}
	return result;
}

void *LoadFile(const char *fileName, int* size)
{
	bool result = false;
	FILE *f = nullptr;
	int _size;
	void *ptr = NULL;

	if (fopen_s(&f, fileName, "rb")) {
		return nullptr;
	}

	if (fseek(f, 0, SEEK_END)) {
		goto DONE;
	}
	_size = ftell(f);
	if (_size < 0) {
		goto DONE;
	}
	if (fseek(f, 0, SEEK_SET)) {
		goto DONE;
	}
	ptr = calloc(_size + 1, 1);
	if (!ptr) {
		goto DONE;
	}
	if (_size > 0) {
		if (!fread(ptr, _size, 1, f)) {
			goto DONE;
		}
	}
	result = true;
	if (size) {
		*size = _size;
	}
DONE:
	if (f) {
		fclose(f);
	}
	if (result){
		return ptr;
	} else {
		if (ptr) {
			free(ptr);
		}
		return nullptr;
	}
}

void GoMyDir()
{
	char dir[MAX_PATH];
	GetModuleFileNameA(GetModuleHandleA(nullptr), dir, MAX_PATH);
	char* p = strrchr(dir, '\\');
	assert(p);
	*p = '\0';
	SetCurrentDirectoryA(dir);
	SetCurrentDirectoryA("../../pack/assets");
}

#pragma comment(lib, "winmm.lib")
#include <mmsystem.h>
void PlayBgm(const char* fileName)
{
//	PlaySoundA(fileName, NULL, SND_ASYNC | SND_LOOP);
	mciSendStringA(SPrintf("open \"%s\" type mpegvideo", fileName), NULL, 0, 0);
	mciSendStringA(SPrintf("play \"%s\" repeat", fileName), NULL, 0, 0);
}

static UINT StrToType(const char* type)
{
	if (!strcmp(type, "okcancel")) {
		return MB_OKCANCEL;
	} else if (!strcmp(type, "yesno")) {
		return MB_YESNO;
	}
	return MB_OK;
}

static const char* IdToStr(int id)
{
	switch (id) {
	case IDOK: return "ok";
	case IDCANCEL: return "cancel";
	case IDYES: return "yes";
	case IDNO: return "no";
	}
	return "unknown";
}

const char* StrMessageBox(const char* txt, const char* type)
{
	return IdToStr(MessageBoxA(GetActiveWindow(), txt, "MessageBox", StrToType(type)));
}

namespace Gdiplus {
	using std::min;
	using std::max;
}
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

bool LoadImageViaGdiPlus(const char* name, IVec2& size, std::vector<uint32_t>& col)
{
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
	WCHAR wc[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, name, -1, wc, dimof(wc));
	Gdiplus::Bitmap* image = new Gdiplus::Bitmap(wc);

	int w = (int)image->GetWidth();
	int h = (int)image->GetHeight();
	size.x = w;
	size.y = h;
	Gdiplus::Rect rc(0, 0, w, h);

	Gdiplus::BitmapData* bitmapData = new Gdiplus::BitmapData;
	image->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);

	col.resize(w * h);
	for (int y = 0; y < h; y++) {
		memcpy(&col[y * w], (char*)bitmapData->Scan0 + bitmapData->Stride * y, w * 4);
		for (int x = 0; x < w; x++) {
			uint32_t& c = col[y * w + x];
			c = (c & 0xff00ff00) | ((c & 0xff) << 16) | ((c & 0xff0000) >> 16);
		}
	}
	image->UnlockBits(bitmapData);
	delete bitmapData;
	delete image;
	Gdiplus::GdiplusShutdown(gdiplusToken);
	return w && h;
}

SRVID LoadTextureViaOS(const char* name, IVec2& size)
{
	std::vector<uint32_t> col;
	if (!LoadImageViaGdiPlus(name, size, col)) {
		return SRVID();
	}
	return afCreateTexture2D(AFDT_R8G8B8A8_UNORM, size, &col[0]);
}

#define IS_HANGUL(c)	( (c) >= 0xAC00 && (c) <= 0xD7A3 )
#define IS_HANGUL2(c)	( ( (c) >= 0x3130 && (c) <= 0x318F ) || IS_HANGUL(c) )	// hangul + jamo

static bool isKorean(int code)
{
	return IS_HANGUL2(code) || code < 0x80;
}

static HFONT CreateAsianFont(int code, int height)
{
	BOOL isK = isKorean(code);
	const char *fontName = isK ? "Gulim" : "MS Gothic";
	const DWORD charset = isK ? HANGUL_CHARSET : SHIFTJIS_CHARSET;
	return CreateFontA(height,			// Height Of Font
		0,								// Width Of Font
		0,								// Angle Of Escapement
		0,								// Orientation Angle
		FW_MEDIUM,						// Font Weight
		FALSE,							// Italic
		FALSE,							// Underline
		FALSE,							// Strikeout
		charset,						// Character Set Identifier
		OUT_TT_PRECIS,					// Output Precision
		CLIP_DEFAULT_PRECIS,			// Clipping Precision
		ANTIALIASED_QUALITY,			// Output Quality
		FF_DONTCARE | DEFAULT_PITCH,	// Family And Pitch
		fontName);						// Font Name
}

void MakeFontBitmap(const char* fontName, const CharSignature& sig, DIB& dib, CharDesc& cache)
{
	bool result = false;

	HFONT font = CreateAsianFont(sig.code, sig.fontSize);
	assert(font);

	dib.Clear();
	wchar_t buf[] = { sig.code, '\0' };
	HDC hdc = GetDC(nullptr);
	assert(hdc);
	HFONT oldFont = (HFONT)SelectObject(hdc, font);
	const MAT2 mat = { { 0,1 },{ 0,0 },{ 0,0 },{ 0,1 } };
	GLYPHMETRICS met;
	memset(&met, 0, sizeof(met));
	DWORD sizeReq = GetGlyphOutlineW(hdc, (UINT)sig.code, GGO_GRAY8_BITMAP, &met, 0, nullptr, &mat);
	if (sizeReq) {
		DIB dib3;
		afVerify(dib3.Create(met.gmBlackBoxX, met.gmBlackBoxY, 8, 64));
		afVerify(dib.Create(met.gmBlackBoxX, met.gmBlackBoxY));
		int sizeBuf = dib3.GetByteSize();
		if (sizeReq != sizeBuf) {
			aflog("FontMan::Build() buf size mismatch! code=%d req=%d dib=%d\n", sig.code, sizeReq, sizeBuf);
			int fakeBlackBoxY = met.gmBlackBoxY + 1;
			afVerify(dib3.Create(met.gmBlackBoxX, fakeBlackBoxY, 8, 64));
			afVerify(dib.Create(met.gmBlackBoxX, fakeBlackBoxY));
			int sizeBuf = dib3.GetByteSize();
			if (sizeReq != sizeBuf) {
				afVerify(false);
			} else {
				aflog("FontMan::Build() buf size matched by increasing Y, but it is an awful workaround. code=%d req=%d dib=%d\n", sig.code, sizeReq, sizeBuf);
			}
		}
		memset(&met, 0, sizeof(met));
		GetGlyphOutlineW(hdc, (UINT)sig.code, GGO_GRAY8_BITMAP, &met, sizeReq, dib3.ReferPixels(), &mat);
		//	SetTextColor(hdc, RGB(255, 255, 255));
		//	SetBkColor(hdc, RGB(0, 0, 0));
		//	TextOutW(hdc, 0, 0, buf, wcslen(buf));
		dib3.Blt(dib.GetHDC(), 0, 0, dib3.getW(), dib3.getH());
		//	dib.Save(SPrintf("../ScreenShot/%04x.bmp", sig.code));
		dib.DibToDXFont();
	}
	SelectObject(hdc, (HGDIOBJ)oldFont);
	if (font) {
		DeleteObject(font);
	}
	ReleaseDC(nullptr, hdc);

	cache.srcWidth = Vec2((float)met.gmBlackBoxX, (float)met.gmBlackBoxY);
	cache.step = (float)met.gmCellIncX;
	cache.distDelta = Vec2((float)met.gmptGlyphOrigin.x, (float)-met.gmptGlyphOrigin.y);
}

#endif
