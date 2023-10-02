#ifndef RAAVANAN_H
#include "Raavanan_Platform.h"
#include <math.h>
#include "Raavanan_tile.h"

struct memory_arena
{
	size_t Size;
	uint8 *Base;
	size_t UsedSpace;
};

#define PushStruct(MemoryArena, type) (type *)PushSize_(MemoryArena, sizeof(type))
#define PushArray(MemoryArena, Count, type) (type *)PushSize_(MemoryArena, (Count * sizeof(type)))
void *PushSize_(memory_arena *MemoryArena, size_t Size)
{
	Assert((MemoryArena->UsedSpace + Size) < MemoryArena->Size);
	void *Result = MemoryArena->Base + MemoryArena->UsedSpace;
	MemoryArena->UsedSpace += Size;
	return Result;
}

struct world
{
	tile_map *TileMap;
};

struct loaded_bitmap
{
	int32 Width;
	int32 Height;
	uint32 *Pixels;
};

struct hero_bitmaps
{
	int32 AlignX;
	int32 AlignY;
	loaded_bitmap Head;
	loaded_bitmap Cape;
	loaded_bitmap Torso;
};

struct game_state
{
	world *World;
	memory_arena WorldArena;
	tile_map_position PlayerP;
	tile_map_position CameraP;
	loaded_bitmap Backdrop;

	uint32 HeroFacingDirection;
	hero_bitmaps HeroBitmaps[4];
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