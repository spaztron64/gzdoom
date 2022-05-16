/*
 ** st_console.mm
 **
 **---------------------------------------------------------------------------
 ** Copyright 2015 Alexey Lysiuk
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

#include "i_common.h"
#include "startupinfo.h"
#include "st_console.h"
#include "v_text.h"
#include "version.h"
#include "palentry.h"
#include "v_video.h"
#include "v_font.h"

static NSColor* RGB(const uint8_t red, const uint8_t green, const uint8_t blue)
{
	return [NSColor colorWithCalibratedRed:red   / 255.0f
									 green:green / 255.0f
									  blue:blue  / 255.0f
									 alpha:1.0f];
}

static NSColor* RGB(const PalEntry& color)
{
	return RGB(color.r, color.g, color.b);
}

static NSColor* RGB(const uint32_t color)
{
	return RGB(PalEntry(color));
}


static const CGFloat PROGRESS_BAR_HEIGHT = 18.0f;
static const CGFloat NET_VIEW_HEIGHT     = 88.0f;


FConsoleWindow::FConsoleWindow()
: m_window([NSWindow alloc])
, m_textView([NSTextView alloc])
, m_scrollView([NSScrollView alloc])
, m_progressBar(nil)
, m_netView(nil)
, m_netMessageText(nil)
, m_netCountText(nil)
, m_netProgressBar(nil)
, m_netAbortButton(nil)
, m_characterCount(0)
, m_netCurPos(0)
, m_netMaxPos(0)
{
	const CGFloat initialWidth  = 512.0f;
	const CGFloat initialHeight = 384.0f;
	const NSRect initialRect = NSMakeRect(0.0f, 0.0f, initialWidth, initialHeight);

	[m_textView initWithFrame:initialRect];
	[m_textView setEditable:NO];
	[m_textView setBackgroundColor:RGB(70, 70, 70)];
	[m_textView setMinSize:NSMakeSize(0.0f, initialHeight)];
	[m_textView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
	[m_textView setVerticallyResizable:YES];
	[m_textView setHorizontallyResizable:NO];
	[m_textView setAutoresizingMask:NSViewWidthSizable];

	NSTextContainer* const textContainer = [m_textView textContainer];
	[textContainer setContainerSize:NSMakeSize(initialWidth, FLT_MAX)];
	[textContainer setWidthTracksTextView:YES];

	[m_scrollView initWithFrame:initialRect];
	[m_scrollView setBorderType:NSNoBorder];
	[m_scrollView setHasVerticalScroller:YES];
	[m_scrollView setHasHorizontalScroller:NO];
	[m_scrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	[m_scrollView setDocumentView:m_textView];

	NSString* const title = [NSString stringWithFormat:@"%s %s - Console", GAMENAME, GetVersionString()];

	[m_window initWithContentRect:initialRect
						styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
						  backing:NSBackingStoreBuffered
							defer:NO];
	[m_window setMinSize:[m_window frame].size];
	[m_window setShowsResizeIndicator:NO];
	[m_window setTitle:title];
	[m_window center];
	[m_window exitAppOnClose];

	// Do not allow fullscreen mode for this window
	[m_window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];

	[[m_window contentView] addSubview:m_scrollView];

	[m_window makeKeyAndOrderFront:nil];
}


static FConsoleWindow* s_instance;


void FConsoleWindow::CreateInstance()
{
	assert(NULL == s_instance);
	s_instance = new FConsoleWindow;
}

void FConsoleWindow::DeleteInstance()
{
	assert(NULL != s_instance);
	delete s_instance;
	s_instance = NULL;
}

FConsoleWindow& FConsoleWindow::GetInstance()
{
	assert(NULL != s_instance);
	return *s_instance;
}


void FConsoleWindow::Show(const bool visible)
{
	if (visible)
	{
		[m_window orderFront:nil];
	}
	else
	{
		[m_window orderOut:nil];
	}
}

void FConsoleWindow::ShowFatalError(const char* const message)
{
	SetProgressBar(false);
	NetDone();

	const CGFloat textViewWidth = [m_scrollView frame].size.width;

	ExpandTextView(-32.0f);

	NSButton* quitButton = [[NSButton alloc] initWithFrame:NSMakeRect(textViewWidth - 76.0f, 0.0f, 72.0f, 30.0f)];
	[quitButton setAutoresizingMask:NSViewMinXMargin];
	[quitButton setBezelStyle:NSRoundedBezelStyle];
	[quitButton setTitle:@"Quit"];
	[quitButton setKeyEquivalent:@"\r"];
	[quitButton setTarget:NSApp];
	[quitButton setAction:@selector(stopModal)];

	NSView* quitPanel = [[NSView alloc] initWithFrame:NSMakeRect(0.0f, 0.0f, textViewWidth, 32.0f)];
	[quitPanel setAutoresizingMask:NSViewWidthSizable];
	[quitPanel addSubview:quitButton];

	[[m_window contentView] addSubview:quitPanel];
	[m_window orderFront:nil];

	AddText(PalEntry(255,   0,   0), "\nExecution could not continue.\n");
	AddText(PalEntry(255, 255, 170), message);
	AddText("\n");

	ScrollTextToBottom();

	[NSApp runModalForWindow:m_window];
}


static const unsigned int THIRTY_FPS = 33; // milliseconds per update


template <typename Function, unsigned int interval = THIRTY_FPS>
struct TimedUpdater
{
	explicit TimedUpdater(const Function& function)
	{
		extern uint64_t I_msTime();
		const unsigned int currentTime = I_msTime();

		if (currentTime - m_previousTime > interval)
		{
			m_previousTime = currentTime;

			function();

			[[NSRunLoop currentRunLoop] limitDateForMode:NSDefaultRunLoopMode];
		}
	}

	static unsigned int m_previousTime;
};

template <typename Function, unsigned int interval>
unsigned int TimedUpdater<Function, interval>::m_previousTime;

template <typename Function, unsigned int interval = THIRTY_FPS>
static void UpdateTimed(const Function& function)
{
	TimedUpdater<Function, interval> dummy(function);
}


void FConsoleWindow::AddText(const char* message)
{
	PalEntry color(223, 223, 223);

	char buffer[1024] = {};
	size_t pos = 0;
	bool reset = false;

	while (*message != '\0')
	{
		if ((TEXTCOLOR_ESCAPE == *message && 0 != pos)
			|| (pos == sizeof buffer - 1)
			|| reset)
		{
			buffer[pos] = '\0';
			pos = 0;
			reset = false;

			AddText(color, buffer);
		}

		if (TEXTCOLOR_ESCAPE == *message)
		{
			const uint8_t* colorID = reinterpret_cast<const uint8_t*>(message) + 1;
			if ('\0' == *colorID)
			{
				break;
			}

			const EColorRange range = V_ParseFontColor(colorID, CR_UNTRANSLATED, CR_YELLOW);

			if (range != CR_UNDEFINED)
			{
				color = V_LogColorFromColorRange(range);
			}

			message += 2;
		}
		else if (0x1d == *message || 0x1f == *message) // Opening and closing bar characters
		{
			buffer[pos++] = '-';
			++message;
		}
		else if (0x1e == *message) // Middle bar character
		{
			buffer[pos++] = '=';
			++message;
		}
		else
		{
			buffer[pos++] = *message++;
		}
	}

	if (0 != pos)
	{
		buffer[pos] = '\0';

		AddText(color, buffer);
	}

	if ([m_window isVisible])
	{
		UpdateTimed([&]()
		{
			[m_textView scrollRangeToVisible:NSMakeRange(m_characterCount, 0)];
		});
	}
}

void FConsoleWindow::AddText(const PalEntry& color, const char* const message)
{
	NSString* const text = [NSString stringWithCString:message
											  encoding:NSISOLatin1StringEncoding];

	NSDictionary* const attributes = [NSDictionary dictionaryWithObjectsAndKeys:
									  [NSFont systemFontOfSize:14.0f], NSFontAttributeName,
									  RGB(color), NSForegroundColorAttributeName,
									  nil];

	NSAttributedString* const formattedText =
	[[NSAttributedString alloc] initWithString:text
									attributes:attributes];
	[[m_textView textStorage] appendAttributedString:formattedText];

	m_characterCount += [text length];
}


void FConsoleWindow::ScrollTextToBottom()
{
	[m_textView scrollRangeToVisible:NSMakeRange(m_characterCount, 0)];

	[[NSRunLoop currentRunLoop] limitDateForMode:NSDefaultRunLoopMode];
}


void FConsoleWindow::ExpandTextView(const float height)
{
	NSRect textFrame = [m_scrollView frame];
	textFrame.origin.y    -= height;
	textFrame.size.height += height;
	[m_scrollView setFrame:textFrame];
}


