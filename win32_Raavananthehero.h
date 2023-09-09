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

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct Win32_RecordingState
{
	uint64 TotalSize;
	void *GameMemoryBlock;
	HANDLE RecordingHanlde;
	int InputRecordingIndex = 0;
	HANDLE PlaybackHandle;
	int InputPlayingIndex = 0;	

	char ExeFileName[WIN32_STATE_FILE_NAME_COUNT];
	char *OnePastLastExeFileNameSlash;
};

#define WIN32_RAAVANANTHEHERO
#endif