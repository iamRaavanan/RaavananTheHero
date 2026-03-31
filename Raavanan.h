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

internal void InitializeArena (memory_arena *MemoryArena, memory_index Size, uint8 *BasePtr)
{
	MemoryArena->Size = Size;
	MemoryArena->Base = BasePtr;
	MemoryArena->UsedSpace = 0;
}


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
	v2 Align;
	loaded_bitmap Head;
	loaded_bitmap Cape;
	loaded_bitmap Torso;
};

enum entity_type
{
	EntityType_None,
	EntityType_Hero,
	EntityType_Wall,
	EntityType_Familiar,
	EntityType_Monster,
	EntityType_Sword
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point
{
	uint8 Flags;
	uint8 FilledAmount;
};

struct high_entity
{
	v2 Pos;
	v2 dPlayerP;
	float Z;
	float dZ;
	uint32 ChunkZ;
	uint32 FacingDirection;
	float tBob;
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

	uint32 HitPointMax;
	hit_point HitPoint[16];

	uint32 SwordLowIndex;
};

struct add_low_entity_result
{
	low_entity* Low;
	uint32 LowIndex;
};

struct entity
{
	uint32 LowIndex;
	low_entity* Low;
	high_entity* High;
};

struct game_state
{
	world *World;
	memory_arena WorldArena;	
	
	float MetersToPixel;
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

	loaded_bitmap Tree;
	loaded_bitmap Sword;
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

struct entity_visible_piece
{
	loaded_bitmap* Bitmap;
	v2 Offset;
	float OffsetZ;
	float EntityZCofficient;
	float R, G, B, A;
	v2 Dim;
};

struct entity_visible_piece_group
{
	uint32 PieceCount;
	entity_visible_piece Pieces[8];
	game_state* GameState;
};

static void UpdateSound(game_state *GameState, game_sound_buffer *SoundBuffer);


#define RAAVANAN_H
#endif