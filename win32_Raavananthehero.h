#ifndef WIN32_RAAVANANTHEHERO
#define WIN32_RAAVANANTHEHERO
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
	int SecondaryBufferSize;
	float tSine;
	int LatencySampleCount;
};

#endif