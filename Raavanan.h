#ifndef RAAVANAN_H
#include "Raavanan_Platform.h"

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

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
#include "Raavanan_world.h"

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

enum entity_type
{
	EntityType_None,
	EntityType_Hero,
	EntityType_Wall
};

struct high_entity
{
	v2 Pos;
	v2 dPlayerP;
	float Z;
	float dZ;
	uint32 AbsTileZ;
	uint32 FacingDirection;

	uint32 LowEntityIndex;
};

struct low_entity
{
	entity_type Type;
	world_position Pos;
	float Height;
	float Width;
	bool Collides;
	int32 dAbsTileZ;

	uint32 HighEntityIndex;
};

struct entity
{
	uint32 LowIndex;
	low_entity* Low;
	high_entity* High;
};

struct low_entity_chunk_reference
{
	world_chunk *TileChunk;
	uint32 EntityIndexInChunk;
};

struct game_state
{
	world *World;
	memory_arena WorldArena;	

	uint32 CameraFollowingEntityIndex;
	world_position CameraP;

	uint32 PlayerControllerIndex[ArrayCount(((game_input *)0)->Controllers)];
	uint32 EntityCount;
	uint32 HighEntityCount;
	high_entity HighEntities[256];
	uint32 LowEntityCount;
	low_entity LowEntities[100000];

	loaded_bitmap Backdrop;
	loaded_bitmap Shadow;
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