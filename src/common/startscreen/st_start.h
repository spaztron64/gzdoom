#pragma once

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

	void Progress() override;
	void NetInit(const char* message, int num_players) override;
	void NetProgress(int count) override;
	void NetMessage(const char* format, ...) override;	// cover for printf
	void NetDone() override;
	bool NetLoop(bool (*timer_callback)(void*), void* userdata) override;
protected:
	long long NetMarqueeMode;
	int NetMaxPos, NetCurPos;
};

class FStartScreen;

extern FStartupScreen *StartScreen;

void DeleteStartupScreen();
extern void ST_Endoom();

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


