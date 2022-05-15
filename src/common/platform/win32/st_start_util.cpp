/*
** st_start.cpp
** Handles the startup screen.
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
*/


#include <stdint.h>
#include <algorithm>
#include "st_start.h"
#include "m_alloc.h"
#include "filesystem.h"
#include "s_soundinternal.h"
#include "s_music.h"
#include "startupinfo.h"
#include "palutil.h"
#include "i_interface.h"

uint8_t* GetHexChar(int codepoint);

void I_GetEvent();	// i_input.h pulls in too much garbage.

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

// Heretic startup screen
#define HERETIC_MINOR_VERSION	'3'			// Since we're based on Heretic 1.3

#define THERM_X					14
#define THERM_Y					14
#define THERM_LEN				51
#define THERM_COLOR				0xA		// light green

// Strife startup screen
#define PEASANT_INDEX			0
#define LASER_INDEX				4
#define BOT_INDEX				6

#define ST_LASERSPACE_X			60
#define ST_LASERSPACE_Y			156
#define ST_LASERSPACE_WIDTH		200
#define ST_LASER_WIDTH			16
#define ST_LASER_HEIGHT			16

#define ST_BOT_X				14
#define ST_BOT_Y				138
#define ST_BOT_WIDTH			48
#define ST_BOT_HEIGHT			48

#define ST_PEASANT_X			262
#define ST_PEASANT_Y			136
#define ST_PEASANT_WIDTH		32
#define ST_PEASANT_HEIGHT		64


static const char* StrifeStartupPicNames[4 + 2 + 1] =
{
	"STRTPA1", "STRTPB1", "STRTPC1", "STRTPD1",
	"STRTLZ1", "STRTLZ2",
	"STRTBOT"
};
static const int StrifeStartupPicSizes[4 + 2 + 1] =
{
	2048, 2048, 2048, 2048,
	256, 256,
	2304
};


static void ST_Sound(const char* sndname)
{
	if (sysCallbacks.PlayStartupSound)
		sysCallbacks.PlayStartupSound(sndname);
}

//==========================================================================
//
// FHexenStartupScreen Constructor
//
// Shows the Hexen startup screen. If the screen doesn't appear to be
// valid, it sets hr for a failure.
//
// The startup graphic is a planar, 4-bit 640x480 graphic preceded by a
// 16 entry (48 byte) VGA palette.
//
//==========================================================================

FHexenStartupScreen::FHexenStartupScreen(int max_progress, long& hr)
	: FGraphicalStartupScreen(max_progress)
{
	int startup_lump = fileSystem.CheckNumForName("STARTUP");
	int netnotch_lump = fileSystem.CheckNumForName("NETNOTCH");
	int notch_lump = fileSystem.CheckNumForName("NOTCH");
	hr = -1;

	if (startup_lump < 0 || fileSystem.FileLength(startup_lump) != 153648 || !ST_Util_CreateStartupWindow() ||
		netnotch_lump < 0 || fileSystem.FileLength(netnotch_lump) != ST_NETNOTCH_WIDTH / 2 * ST_NETNOTCH_HEIGHT ||
		notch_lump < 0 || fileSystem.FileLength(notch_lump) != ST_NOTCH_WIDTH / 2 * ST_NOTCH_HEIGHT)
	{
		NetNotchBits = NotchBits = NULL;
		return;
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

	StartupBitmap = ST_Util_CreateBitmap(640, 480);

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
	ST_Util_PlanarToChunky4(ST_Util_BitsForBitmap(StartupBitmap), startup_screen + 48, 640, 480);


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
	SetWindowSize();
	hr = 0;
}

//==========================================================================
//
// FHexenStartupScreen Deconstructor
//
// Frees the notch pictures.
//
//==========================================================================

FHexenStartupScreen::~FHexenStartupScreen()
{
	if (NotchBits)
		delete[] NotchBits;
	if (NetNotchBits)
		delete[] NetNotchBits;
}

//==========================================================================
//
// FHexenStartupScreen :: Progress
//
// Bumps the progress meter one notch.
//
//==========================================================================

void FHexenStartupScreen::Progress()
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
				ST_Util_DrawBlock4(StartupBitmap, NotchBits, x, y, ST_NOTCH_WIDTH / 2, ST_NOTCH_HEIGHT);
			}
			ST_Sound("StartupTick");
		}
	}
	I_GetEvent();
}

//==========================================================================
//
// FHexenStartupScreen :: NetProgress
//
// Draws the red net noches in addition to the normal progress bar.
//
//==========================================================================

void FHexenStartupScreen::NetProgress(int count)
{
	int oldpos = NetCurPos;
	int x, y;

	FGraphicalStartupScreen::NetProgress(count);
	if (NetMaxPos != 0 && NetCurPos > oldpos)
	{
		for (; oldpos < NetCurPos && oldpos < ST_MAX_NETNOTCHES; ++oldpos)
		{
			x = ST_NETPROGRESS_X + ST_NETNOTCH_WIDTH * oldpos;
			y = ST_NETPROGRESS_Y;
			ST_Util_DrawBlock4(StartupBitmap, NetNotchBits, x, y, ST_NETNOTCH_WIDTH / 2, ST_NETNOTCH_HEIGHT);
		}
		ST_Sound("misc/netnotch");
		I_GetEvent();
	}
}

//==========================================================================
//
// FHexenStartupScreen :: NetDone
//
// Aside from the standard processing, also plays a sound.
//
//==========================================================================

void FHexenStartupScreen::NetDone()
{
	ST_Sound("PickupWeapon");
	FGraphicalStartupScreen::NetDone();
}


//==========================================================================
//
// FHereticStartupScreen Constructor
//
// Shows the Heretic startup screen. If the screen doesn't appear to be
// valid, it returns a failure code in hr.
//
// The loading screen is an 80x25 text screen with character data and
// attributes intermixed, which means it must be exactly 4000 bytes long.
//
//==========================================================================

FHereticStartupScreen::FHereticStartupScreen(int max_progress, long& hr)
	: FGraphicalStartupScreen(max_progress)
{
	int loading_lump = fileSystem.CheckNumForName("LOADING");
	uint8_t loading_screen[4000];

	hr = -1;
	if (loading_lump < 0 || fileSystem.FileLength(loading_lump) != 4000 || !ST_Util_CreateStartupWindow())
	{
		return;
	}

	fileSystem.ReadFile(loading_lump, loading_screen);

	// Slap the Heretic minor version on the loading screen. Heretic
	// did this inside the executable rather than coming with modified
	// LOADING screens, so we need to do the same.
	loading_screen[2 * 160 + 49 * 2] = HERETIC_MINOR_VERSION;

	// Draw the loading screen to a bitmap.
	StartupBitmap = ST_Util_AllocTextBitmap();
	ST_Util_DrawTextScreen(StartupBitmap, loading_screen);

	ThermX = THERM_X * 8;
	ThermY = THERM_Y * 16;
	ThermWidth = THERM_LEN * 8 - 4;
	ThermHeight = 16;
	HMsgY = 7;
	SMsgX = 1;

	SetWindowSize();
	hr = 0;
}

//==========================================================================
//
// FHereticStartupScreen::Progress
//
// Bumps the progress meter one notch.
//
//==========================================================================

void FHereticStartupScreen::Progress()
{
	int notch_pos;

	if (CurPos < MaxPos)
	{
		CurPos++;
		notch_pos = (CurPos * ThermWidth) / MaxPos;
		if (notch_pos != NotchPos && !(notch_pos & 3))
		{ // Time to draw another notch.
			int left = NotchPos + ThermX;
			int top = ThermY;
			int right = notch_pos + ThermX;
			int bottom = top + ThermHeight;
			ST_Util_ClearBlock(StartupBitmap, THERM_COLOR, left, top, right - left, bottom - top);
			NotchPos = notch_pos;
		}
	}
	I_GetEvent();
}

//==========================================================================
//
// FHereticStartupScreen :: LoadingStatus
//
// Prints text in the center box of the startup screen.
//
//==========================================================================

void FHereticStartupScreen::LoadingStatus(const char* message, int colors)
{
	int x;

	for (x = 0; message[x] != '\0'; ++x)
	{
		ST_Util_DrawChar(StartupBitmap, 17 + x, HMsgY, message[x], colors);
	}
	ST_Util_InvalidateRect(StartupBitmap, 17 * 8, HMsgY * 16, (17 + x) * 8, HMsgY * 16 + 16);
	HMsgY++;
	I_GetEvent();
}

//==========================================================================
//
// FHereticStartupScreen :: AppendStatusLine
//
// Appends text to Heretic's status line.
//
//==========================================================================

void FHereticStartupScreen::AppendStatusLine(const char* status)
{
	int x;

	for (x = 0; status[x] != '\0'; ++x)
	{
		ST_Util_DrawChar(StartupBitmap, SMsgX + x, 24, status[x], 0x1f);
	}
	ST_Util_InvalidateRect(StartupBitmap, SMsgX * 8, 24 * 16, (SMsgX + x) * 8, 25 * 16);
	SMsgX += x;
	I_GetEvent();
}

//==========================================================================
//
// FStrifeStartupScreen Constructor
//
// Shows the Strife startup screen. If the screen doesn't appear to be
// valid, it returns a failure code in hr.
//
// The startup background is a raw 320x200 image, however Strife only
// actually uses 95 rows from it, starting at row 57. The rest of the image
// is discarded. (What a shame.)
//
// The peasants are raw 32x64 images. The laser dots are raw 16x16 images.
// The bot is a raw 48x48 image. All use the standard PLAYPAL.
//
//==========================================================================

FStrifeStartupScreen::FStrifeStartupScreen(int max_progress, long& hr)
	: FGraphicalStartupScreen(max_progress)
{
	int startup_lump = fileSystem.CheckNumForName("STARTUP0");
	int i;

	hr = -1;
	for (i = 0; i < 4 + 2 + 1; ++i)
	{
		StartupPics[i] = NULL;
	}

	if (startup_lump < 0 || fileSystem.FileLength(startup_lump) != 64000 || !ST_Util_CreateStartupWindow())
	{
		return;
	}

	StartupBitmap = ST_Util_CreateBitmap(320, 200);
	ST_Util_BitmapColorsFromPlaypal(StartupBitmap);

	// Fill bitmap with the startup image.
	memset(ST_Util_BitsForBitmap(StartupBitmap), 0xF0, 64000);
	auto lumpr = fileSystem.OpenFileReader(startup_lump);
	lumpr.Seek(57 * 320, FileReader::SeekSet);
	lumpr.Read(ST_Util_BitsForBitmap(StartupBitmap) + 41 * 320, 95 * 320);

	// Load the animated overlays.
	for (i = 0; i < 4 + 2 + 1; ++i)
	{
		int lumpnum = fileSystem.CheckNumForName(StrifeStartupPicNames[i]);
		int lumplen;

		if (lumpnum >= 0 && (lumplen = fileSystem.FileLength(lumpnum)) == StrifeStartupPicSizes[i])
		{
			auto lumpr1 = fileSystem.OpenFileReader(lumpnum);
			StartupPics[i] = new uint8_t[lumplen];
			lumpr1.Read(StartupPics[i], lumplen);
		}
	}

	// Make the startup image appear.
	DrawStuff(0, 0);
	SetWindowSize();
	hr = 0;
}

//==========================================================================
//
// FStrifeStartupScreen Destructor
//
// Frees the strife pictures.
//
//==========================================================================

FStrifeStartupScreen::~FStrifeStartupScreen()
{
	for (int i = 0; i < 4 + 2 + 1; ++i)
	{
		if (StartupPics[i] != NULL)
		{
			delete[] StartupPics[i];
		}
		StartupPics[i] = NULL;
	}
}

//==========================================================================
//
// FStrifeStartupScreen :: Progress
//
// Bumps the progress meter one notch.
//
//==========================================================================

void FStrifeStartupScreen::Progress()
{
	int notch_pos;

	if (CurPos < MaxPos)
	{
		CurPos++;
		notch_pos = (CurPos * (ST_LASERSPACE_WIDTH - ST_LASER_WIDTH)) / MaxPos;
		if (notch_pos != NotchPos && !(notch_pos & 1))
		{ // Time to update.
			DrawStuff(NotchPos, notch_pos);
			NotchPos = notch_pos;
		}
	}
	I_GetEvent();
}

//==========================================================================
//
// FStrifeStartupScreen :: DrawStuff
//
// Draws all the moving parts of Strife's startup screen. If you're
// running off a slow drive, it can look kind of good. Otherwise, it
// borders on crazy insane fast.
//
//==========================================================================

void FStrifeStartupScreen::DrawStuff(int old_laser, int new_laser)
{
	int y;
	auto bitmap_info = StartupBitmap;

	// Clear old laser
	ST_Util_ClearBlock(bitmap_info, 0xF0, ST_LASERSPACE_X + old_laser,
		ST_LASERSPACE_Y, ST_LASER_WIDTH, ST_LASER_HEIGHT);
	// Draw new laser
	ST_Util_DrawBlock(bitmap_info, StartupPics[LASER_INDEX + (new_laser & 1)],
		ST_LASERSPACE_X + new_laser, ST_LASERSPACE_Y, ST_LASER_WIDTH, ST_LASER_HEIGHT);

	// The bot jumps up and down like crazy.
	y = max(0, (new_laser >> 1) % 5 - 2);
	if (y > 0)
	{
		ST_Util_ClearBlock(bitmap_info, 0xF0, ST_BOT_X, ST_BOT_Y, ST_BOT_WIDTH, y);
	}
	ST_Util_DrawBlock(bitmap_info, StartupPics[BOT_INDEX], ST_BOT_X, ST_BOT_Y + y, ST_BOT_WIDTH, ST_BOT_HEIGHT);
	if (y < (5 - 1) - 2)
	{
		ST_Util_ClearBlock(bitmap_info, 0xF0, ST_BOT_X, ST_BOT_Y + ST_BOT_HEIGHT + y, ST_BOT_WIDTH, 2 - y);
	}

	// The peasant desperately runs in place, trying to get away from the laser.
	// Yet, despite all his limb flailing, he never manages to get anywhere.
	ST_Util_DrawBlock(bitmap_info, StartupPics[PEASANT_INDEX + ((new_laser >> 1) & 3)],
		ST_PEASANT_X, ST_PEASANT_Y, ST_PEASANT_WIDTH, ST_PEASANT_HEIGHT);
}


