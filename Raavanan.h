#ifndef RAAVANAN_H
#include "Raavanan_Platform.h"

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

#include <math.h>
#include "Raavanan_math.h"
#include "Raavanan_tile.h"
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

struct entity
{
	bool Exists;
	float Height;
	float Width;
	tile_map_position Pos;
	v2 dPlayerP;
	uint32 FacingDirection;
};

struct game_state
{
	world *World;
	memory_arena WorldArena;	

	uint32 CameraFollowingEntityIndex;
	tile_map_position CameraP;

	uint32 PlayerControllerIndex[ArrayCount(((game_input *)0)->Controllers)];
	uint32 EntityCount;
	entity Entities[256];

	loaded_bitmap Backdrop;
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