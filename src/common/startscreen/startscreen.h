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
#include "bitmap.h"

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
	RgbQuad             bmiColors[1];
};

extern const RgbQuad TextModePalette[16];

class FStartScreen
{
protected:
	int CurPos = 0;
	int MaxPos;
	int Scale = 1;
	int NetMaxPos = -1;
	int NetCurPos = 0;
	FBitmap StartupBitmap;
	FBitmap HeaderBitmap;
	FBitmap NetBitmap;
	FString NetMessageString;
	FGameTexture* StartupTexture = nullptr;
	FGameTexture* HeaderTexture = nullptr;
	FGameTexture* NetTexture = nullptr;
public:
	FStartScreen(int maxp) { MaxPos = maxp; }
	virtual ~FStartScreen() = default;
	virtual bool Progress();
	virtual void LoadingStatus(const char *message, int colors) {}
	virtual void AppendStatusLine(const char *status) {}
	virtual bool NetInit(const char* message, int numplayers);
	virtual bool NetMessage(const char* format, ...) { return false; }
	virtual void NetProgress(int count) 
	{
		if (count == 0)
		{
			NetCurPos++;
		}
		else if (count > 0)
		{
			NetCurPos = count;
		}
	}
	virtual void NetDone() {}
	virtual void NetTick() {}
	FBitmap& GetBitmap() { return StartupBitmap; }
	int GetScale() const { return Scale; }

	
protected:
	void ClearBlock(FBitmap& bitmap_info, RgbQuad fill, int x, int y, int bytewidth, int height);
	FBitmap AllocTextBitmap();
	void DrawTextScreen(FBitmap& bitmap_info, const uint8_t* text_screen);
	int DrawChar(FBitmap& screen, int x, int y, unsigned charnum, uint8_t attrib);
	int DrawChar(FBitmap& screen, int x, int y, unsigned charnum, RgbQuad fg, RgbQuad bg);
	int DrawString(FBitmap& screen, int x, int y, const char* text, RgbQuad fg, RgbQuad bg);
	void UpdateTextBlink(FBitmap& bitmap_info, const uint8_t* text_screen, bool on);
	void ST_Sound(const char* sndname);
	int SizeOfText(const char* text);
	void CreateHeader();
	void DrawNetStatus(int found, int total);
	void InvalidateTexture();
};
