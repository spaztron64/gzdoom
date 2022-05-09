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
** The startup screen interface is based on a mix of Heretic and Hexen.
** Actual implementation is system-specific.
*/
#include <stdint.h>
#include "startscreen.h"

class FStartupScreen
{
public:
	static FStartupScreen *CreateInstance(int max_progress);

	FStartupScreen(int max_progress)
	{
		MaxPos = max_progress;
		CurPos = 0;
		NotchPos = 0;
	}

	virtual ~FStartupScreen() = default;

	virtual void Progress() {}

	virtual void NetInit(const char *message, int num_players) {}
	virtual void NetProgress(int count) {}
	virtual void NetMessage(const char *format, ...) {}	// cover for printf
	virtual void NetDone() {}
	virtual bool NetLoop(bool (*timer_callback)(void *), void *userdata) { return false; }
	virtual void AppendStatusLine(const char* status) {}
	virtual void LoadingStatus(const char* message, int colors) {}

protected:
	int MaxPos, CurPos, NotchPos;
};

class FBasicStartupScreen : public FStartupScreen
{
public:
	FBasicStartupScreen(int max_progress, bool show_bar);
	~FBasicStartupScreen();

	void Progress();
	void NetInit(const char* message, int num_players);
	void NetProgress(int count);
	void NetMessage(const char* format, ...);	// cover for printf
	void NetDone();
	bool NetLoop(bool (*timer_callback)(void*), void* userdata);
protected:
	long long NetMarqueeMode;
	int NetMaxPos, NetCurPos;
};

class FGraphicalStartupScreen : public FBasicStartupScreen
{
public:
	FGraphicalStartupScreen(int max_progress);
	~FGraphicalStartupScreen();
};

class FHereticStartupScreen : public FGraphicalStartupScreen
{
public:
	FHereticStartupScreen(int max_progress, long &hr);

	void Progress();
	void LoadingStatus(const char *message, int colors);
	void AppendStatusLine(const char *status);
protected:
	void SetWindowSize();

	int ThermX, ThermY, ThermWidth, ThermHeight;
	int HMsgY, SMsgX;
};

class FHexenStartupScreen : public FGraphicalStartupScreen
{
public:
	FHexenStartupScreen(int max_progress, long &hr);
	~FHexenStartupScreen();

	void Progress();
	void NetProgress(int count);
	void NetDone();
	void SetWindowSize();

	// Hexen's notch graphics, converted to chunky pixels.
	uint8_t * NotchBits;
	uint8_t * NetNotchBits;
};

class FStrifeStartupScreen : public FGraphicalStartupScreen
{
public:
	FStrifeStartupScreen(int max_progress, long &hr);
	~FStrifeStartupScreen();

	void Progress();
protected:
	void DrawStuff(int old_laser, int new_laser);
	void SetWindowSize();

	uint8_t *StartupPics[4+2+1];
};



extern FStartupScreen *StartScreen;

void DeleteStartupScreen();
extern void ST_Endoom();

extern BitmapInfo* StartupBitmap;


//===========================================================================
//
// DeleteStartupScreen
//
// Makes sure the startup screen has been deleted before quitting.
//
//===========================================================================

inline void DeleteStartupScreen()
{
	if (StartScreen != nullptr)
	{
		delete StartScreen;
		StartScreen = nullptr;
	}
}


