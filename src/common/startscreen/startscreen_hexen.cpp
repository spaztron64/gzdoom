/*
** st_start.cpp
** Handles the startup screen.
**
**---------------------------------------------------------------------------
** Copyright 2006-2007 Randy Heit
** Copyright 2006-2022 Christoph Oelckers
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
*/

#include "startscreen.h"
#include "filesystem.h"
#include "printf.h"
#include "startupinfo.h"
#include "s_music.h"

// Hexen startup screen
#define ST_MAX_NOTCHES			32
#define ST_NOTCH_WIDTH			16
#define ST_NOTCH_HEIGHT			23
#define ST_PROGRESS_X			64			// Start of notches x screen pos.
#define ST_PROGRESS_Y			441			// Start of notches y screen pos.

#define ST_NETPROGRESS_X		288
#define ST_NETPROGRESS_Y		32
#define ST_NETNOTCH_WIDTH		4
#define ST_NETNOTCH_HEIGHT		16
#define ST_MAX_NETNOTCHES		8

class FHexenStartScreen : public FStartScreen
{
	BitmapInfo* StartupBitmap = nullptr;
	// Hexen's notch graphics, converted to chunky pixels.
	uint8_t * NotchBits = nullptr;
	uint8_t * NetNotchBits = nullptr;
	int NotchPos = 0;
	int NetCurPos = 0;
	int NetMaxPos = 0;

public:
	FHexenStartScreen(int max_progress, InvalidateRectFunc& inv);
	~FHexenStartScreen();

	bool Progress() override;
	void NetProgress(int count) override;
	void NetDone() override;
	BitmapInfo* GetBitmap() override { return StartupBitmap; }
};


//==========================================================================
//
// FHexenStartScreen Constructor
//
// Shows the Hexen startup screen. If the screen doesn't appear to be
// valid, it sets hr for a failure.
//
// The startup graphic is a planar, 4-bit 640x480 graphic preceded by a
// 16 entry (48 byte) VGA palette.
//
//==========================================================================

FHexenStartScreen::FHexenStartScreen(int max_progress, InvalidateRectFunc& inv)
	: FStartScreen(max_progress, inv)
{
	int startup_lump = fileSystem.CheckNumForName("STARTUP");
	int netnotch_lump = fileSystem.CheckNumForName("NETNOTCH");
	int notch_lump = fileSystem.CheckNumForName("NOTCH");

	if (startup_lump < 0 || fileSystem.FileLength(startup_lump) != 153648 ||
		netnotch_lump < 0 || fileSystem.FileLength(netnotch_lump) != ST_NETNOTCH_WIDTH / 2 * ST_NETNOTCH_HEIGHT ||
		notch_lump < 0 || fileSystem.FileLength(notch_lump) != ST_NOTCH_WIDTH / 2 * ST_NOTCH_HEIGHT)
	{
		I_Error("Start screen assets missing");
	}

	NetNotchBits = new uint8_t[ST_NETNOTCH_WIDTH / 2 * ST_NETNOTCH_HEIGHT];
	fileSystem.ReadFile(netnotch_lump, NetNotchBits);
	NotchBits = new uint8_t[ST_NOTCH_WIDTH / 2 * ST_NOTCH_HEIGHT];
	fileSystem.ReadFile(notch_lump, NotchBits);

	uint8_t startup_screen[153648];
	union
	{
		RgbQuad color;
		uint32_t	quad;
	} c;

	fileSystem.ReadFile(startup_lump, startup_screen);

	c.color.rgbReserved = 0;

	StartupBitmap = CreateBitmap(640, 480);

	// Initialize the bitmap palette.
	for (int i = 0; i < 16; ++i)
	{
		c.color.rgbRed = startup_screen[i * 3 + 0];
		c.color.rgbGreen = startup_screen[i * 3 + 1];
		c.color.rgbBlue = startup_screen[i * 3 + 2];
		// Convert from 6-bit per component to 8-bit per component.
		c.quad = (c.quad << 2) | ((c.quad >> 4) & 0x03030303);
		StartupBitmap->bmiColors[i] = c.color;
	}

	// Fill in the bitmap data. Convert to chunky, because I can't figure out
	// if Windows actually supports planar images or not, despite the presence
	// of biPlanes in the BITMAPINFOHEADER.
	PlanarToChunky4(BitsForBitmap(StartupBitmap), startup_screen + 48, 640, 480);


	if (!batchrun)
	{
		if (GameStartupInfo.Song.IsNotEmpty())
		{
			S_ChangeMusic(GameStartupInfo.Song.GetChars(), true, true);
		}
		else
		{
			S_ChangeMusic("orb", true, true);
		}
	}
}

//==========================================================================
//
// FHexenStartScreen Deconstructor
//
// Frees the notch pictures.
//
//==========================================================================

FHexenStartScreen::~FHexenStartScreen()
{
	FreeBitmap(StartupBitmap);
	if (NotchBits)
		delete[] NotchBits;
	if (NetNotchBits)
		delete[] NetNotchBits;
}

//==========================================================================
//
// FHexenStartScreen :: Progress
//
// Bumps the progress meter one notch.
//
//==========================================================================

bool FHexenStartScreen::Progress()
{
	int notch_pos, x, y;

	if (CurPos < MaxPos)
	{
		CurPos++;
		notch_pos = (CurPos * ST_MAX_NOTCHES) / MaxPos;
		if (notch_pos != NotchPos)
		{ // Time to draw another notch.
			for (; NotchPos < notch_pos; NotchPos++)
			{
				x = ST_PROGRESS_X + ST_NOTCH_WIDTH * NotchPos;
				y = ST_PROGRESS_Y;
				DrawBlock4(StartupBitmap, NotchBits, x, y, ST_NOTCH_WIDTH / 2, ST_NOTCH_HEIGHT);
			}
			ST_Sound("StartupTick");
		}
	}
	return true;
}

//==========================================================================
//
// FHexenStartScreen :: NetProgress
//
// Draws the red net noches in addition to the normal progress bar.
//
//==========================================================================

void FHexenStartScreen::NetProgress(int count)
{
	int oldpos = NetCurPos;
	int x, y;

	FStartScreen::NetProgress(count);
	if (NetMaxPos != 0 && NetCurPos > oldpos)
	{
		for (; oldpos < NetCurPos && oldpos < ST_MAX_NETNOTCHES; ++oldpos)
		{
			x = ST_NETPROGRESS_X + ST_NETNOTCH_WIDTH * oldpos;
			y = ST_NETPROGRESS_Y;
			DrawBlock4(StartupBitmap, NetNotchBits, x, y, ST_NETNOTCH_WIDTH / 2, ST_NETNOTCH_HEIGHT);
		}
		ST_Sound("misc/netnotch");
	}
}

//==========================================================================
//
// FHexenStartScreen :: NetDone
//
// Aside from the standard processing, also plays a sound.
//
//==========================================================================

void FHexenStartScreen::NetDone()
{
	ST_Sound("PickupWeapon");
	FStartScreen::NetDone();
}


FStartScreen* CreateHexenStartScreen(int max_progress, InvalidateRectFunc& func)
{
	return new FHexenStartScreen(max_progress, func);
}
