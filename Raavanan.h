#ifndef RAAVANAN_H
#include "Raavanan_Platform.h"
#include <math.h>
#define PI 3.14159265359

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

inline game_controller_input *GetController (game_input *input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(input->Controllers));
	game_controller_input *Result = &input->Controllers[ControllerIndex];
	return Result;
}

struct canonical_position
{
	int32 TileMapX;
	int32 TileMapY;

	int32 TileX;
	int32 TileY;
	// Tile relative X & Y
	float RelativeX;
	float RelativeY;
};

struct tile_map
{
	uint32 *Tiles;
};

struct world
{
	float TileSideInMeters;
	int32 TileSideInPixels; // Tile width and Height
	float MeterToPixels;
	
	int32 XCount;
	int32 YCount;

	float UpperLeftX;
	float UpperLeftY;
	
	int32 TileMapXCount;
	int32 TileMapYCount;
	tile_map *TileMaps;
};


struct game_state
{
	canonical_position PlayerP;
#if RAAVANAN_INTERNAL
	int ToneHz;
	int XOffset;
	int YOffset;

	float tSine;

	int PlayerX;
	int PlayerY;
	float jumpTime;
#endif
};

static void UpdateSound(game_state *GameState, game_sound_buffer *SoundBuffer);


#define RAAVANAN_H
#endif