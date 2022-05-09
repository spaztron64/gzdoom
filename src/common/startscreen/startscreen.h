#pragma once
/*
** st_start.h
** Interface for the startup screen.
**
**---------------------------------------------------------------------------
** Copyright 2006-2007 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
#pragma once
** The startup screen interface is based on a mix of Heretic and Hexen.
** Actual implementation is system-specific.
*/
#include <stdint.h>
#include <functional>

struct BitmapInfoHeader 
{
	uint32_t      biSize;
	int32_t       biWidth;
	int32_t       biHeight;
	uint16_t      biPlanes;
	uint16_t      biBitCount;
	uint32_t      biCompression;
	uint32_t      biSizeImage;
	int32_t       biXPelsPerMeter;
	int32_t       biYPelsPerMeter;
	uint32_t      biClrUsed;
	uint32_t      biClrImportant;
};

struct RgbQuad 
{
	uint8_t    rgbBlue;
	uint8_t    rgbGreen;
	uint8_t    rgbRed;
	uint8_t    rgbReserved;
};


struct BitmapInfo 
{
	BitmapInfoHeader    bmiHeader;
	RgbQuad             bmiColors[1];
};


using InvalidateRectFunc = std::function<void(BitmapInfo* bmi, int, int, int, int)>;

class FStartScreen
{
protected:
	int CurPos = 0;
	int MaxPos;
	InvalidateRectFunc InvalidateRect;
public:
	FStartScreen(int maxp, InvalidateRectFunc inv) { MaxPos = maxp; InvalidateRect = inv; }
	virtual ~FStartScreen() = default;
	virtual void Progress() 
	{
		if (CurPos < MaxPos)
			++CurPos;
	}
	virtual void LoadingStatus(const char *message, int colors) {}
	virtual void AppendStatusLine(const char *status) {}
	virtual bool NetInit(const char* message, int numplayers) { return false; }
	virtual bool NetMessage(const char* format, ...) { return false; }
	virtual void NetProgress(int) {}
	virtual void NetDone() {}
	virtual void NetTick() {}
	virtual BitmapInfo* GetBitmap() { return nullptr; }
	
protected:
	void PlanarToChunky4(uint8_t* dest, const uint8_t* src, int width, int height);
	void DrawBlock(BitmapInfo* bitmap_info, const uint8_t* src, int x, int y, int bytewidth, int height);
	void DrawBlock4(BitmapInfo* bitmap_info, const uint8_t* src, int x, int y, int bytewidth, int height);
	void ClearBlock(BitmapInfo* bitmap_info, uint8_t fill, int x, int y, int bytewidth, int height);
	BitmapInfo* CreateBitmap(int width, int height);
	uint8_t* BitsForBitmap(BitmapInfo* bitmap_info);
	void FreeBitmap(BitmapInfo* bitmap_info);
	void BitmapColorsFromPlaypal(BitmapInfo* bitmap_info);
	BitmapInfo* AllocTextBitmap();
	void DrawTextScreen(BitmapInfo* bitmap_info, const uint8_t* text_screen);
	int DrawChar(BitmapInfo* screen, int x, int y, unsigned charnum, uint8_t attrib);
	void UpdateTextBlink(BitmapInfo* bitmap_info, const uint8_t* text_screen, bool on);
	void ST_Sound(const char* sndname);

};
