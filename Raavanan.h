#ifndef RAAVANAN_H
#include <math.h>
#include <stdint.h>

#define PI 3.14159265359

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define Assert(Expression) if(!(Expression)) { *(int *) 0 = 0;}

#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

uint32 SafeTruncateUInt64(uint64 value)
{
	Assert(value <= 0xFFFFFFFF);
	uint32 Result = (uint32)value;
	return Result;
}

struct thread_context
{
	int Placeholder;
};

#if RAAVANAN_INTERNAL
struct debug_read_file_result
{
	uint32 ContentSize;
	void *Content;
};

#define DEBUG_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *Filename)
typedef DEBUG_READ_ENTIRE_FILE(debug_read_entire_file);

#define DEBUG_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_FREE_FILE_MEMORY(debug_free_file_memory);

#define DEBUG_WRITE_ENTIRE_FILE(name) bool name (thread_context *Thread, char *Filename, uint32 Memorysize, void *Memory)
typedef DEBUG_WRITE_ENTIRE_FILE(debug_write_entire_file);

#else
#endif

struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct game_sound_buffer 
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

struct game_button_state
{
	int HalfTransitionCount;
	bool EndedDown;
};

struct game_controller_input
{
	bool IsConnected;
	bool IsAnalog;
	float StickAverageX;
	float StickAverageY;
	union 
	{
		game_button_state Buttons[12];
		struct 
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Back;
			game_button_state Start;
		};
	};	
};

struct game_input
{
	game_button_state MouseButtons[5];
	int32 MouseX, MouseY, MouseZ;
	game_controller_input Controllers[5];	// 4 controller + 1 Keyboard
};

inline game_controller_input *GetController (game_input *input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(input->Controllers));
	game_controller_input *Result = &input->Controllers[ControllerIndex];
	return Result;
}

struct game_memory
{
	bool IsInitialized;
	uint64 PermanentStorageSize;
	void *PermanentStorage;
	uint64 TransientStorageSize;
	void *TransientStorage;
#if RAAVANAN_INTERNAL
	debug_read_entire_file *DEBUGReadEntireFile;
	debug_free_file_memory *DEBUGFreeFileMemory;
	debug_write_entire_file *DEBUGWriteEntireFile;
#endif
};

struct game_state
{
	int ToneHz;
	int XOffset;
	int YOffset;

	float tSine;

	int PlayerX;
	int PlayerY;
	float jumpTime;
};

static void UpdateSound(game_state *GameState, game_sound_buffer *SoundBuffer);

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_Render);
// Stub functions are kept for the fallback, We can remove the below stub funciton,
// and assign 0 by default. But, whenever we call that function, we need to do null-check
// before calling the function.
// Ex: if (GameUpdateAndRender) { GameUpdateAndRender(); }
GAME_UPDATE_AND_RENDER(GameUpdateAndRenderStub)
{
}

#define GET_GAME_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory *Memory, game_sound_buffer *SoundBuffer)
typedef GET_GAME_SOUND_SAMPLES(get_game_sound_samples);
GET_GAME_SOUND_SAMPLES(GetGameSoundSamplesStub)
{
}

#define RAAVANAN_H
#endif