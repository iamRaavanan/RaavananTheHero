#ifndef RAAVANAN_H

#define Assert(Expression) if(!(Expression)) { *(int *) 0 = 0;}

#define Kilobytes(value) ((value) * 1024LL)
#define Megabytes(value) (Kilobytes(value) * 1024LL)
#define Gigabytes(value) (Megabytes(value) * 1024LL)
#define Terabytes(value) (Gigabytes(value) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

static uint32 SafeTruncateUInt64(uint64 value)
{
	Assert(value <= 0xFFFFFFFF);
	uint32 Result = (uint32)value;
	return Result;
}

#if RAAVANAN_INTERNAL
struct debug_read_file_result
{
	uint32 ContentSize;
	void *Content;
};
static debug_read_file_result DEBUGReadEntireFile(char *Filename);
static void DEBUGFreeFileMemory(void *Memory);
static bool DEBUGWriteEntireFile(char *Filename, uint32 Memorysize, void *Memory);
#else
#endif

struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
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
};

struct game_state
{
	int ToneHz;
	int XOffset;
	int YOffset;
};

static void UpdateSound(game_sound_buffer *SoundBuffer);
static void GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, game_sound_buffer *SoundBuffer);

#define RAAVANAN_H
#endif