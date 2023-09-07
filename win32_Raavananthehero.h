#ifndef WIN32_RAAVANANTHEHERO
#include <windows.h>

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

struct Win32_Sound_Output
{
	int SamplesPerSecond;
	uint32 CurrentSampleIndex;
	int BytesPerSample;
	DWORD SecondaryBufferSize;
	DWORD SafetyBytes;
	float tSine;
	int LatencySampleCount;
};

struct Win32_debug_time_marker
{
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;
	
	DWORD OutputLocation;	// Output byte to lock
	DWORD OutputByteCount;

	DWORD ExpectedFlipPlayCursor;
	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor;
};

struct Win32_RecordingState
{
	uint64 TotalSize;
	void *GameMemoryBlock;
	HANDLE RecordingHanlde;
	int InputRecordingIndex = 0;
	HANDLE PlaybackHandle;
	int InputPlayingIndex = 0;	
};

#define WIN32_RAAVANANTHEHERO
#endif