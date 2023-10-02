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
struct Win32_Replay_Buffer
{
	HANDLE FileHandle;
	HANDLE MemoryMap;
	char FileName[WIN32_STATE_FILE_NAME_COUNT];
	void *MemoryBlock;
};

struct Win32_RecordingState
{
	uint64 TotalSize;
	void *GameMemoryBlock;

	Win32_Replay_Buffer ReplayBuffers[4];

	HANDLE RecordingHanlde;
	int InputRecordingIndex;

	HANDLE PlaybackHandle;
	int InputPlayingIndex;

	char ExeFileName[WIN32_STATE_FILE_NAME_COUNT];
	char *OnePastLastExeFileNameSlash;
};

#define WIN32_RAAVANANTHEHERO
#endif