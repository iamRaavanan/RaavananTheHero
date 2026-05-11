#ifndef RAAVANAN_H
#include "Raavanan_Platform.h"

/*
TODO:
Architecture Exploration
	- Collision detection
		- Entry/Exit
		- What's the plan for robustness / shape definition?
	- Implement multiple sim regions per frame
		- Per-entity clocking
		- Sim Region merging? For Multiple player
	- Z!
		- Clean up things by using v3
		-  Figure out how you go up and down and how is this rendered?
	- Debug code
		- Logging
		- Programming
		- (A LITTLE GUI, but only a little) Switches/Sliders
	-Audio
		- Sound effect triggers
		- Ambient sounds
		- Music
	-MetaGame/ SaveGame
		- How do you enter "Save Slot"?
		-Persistent Unlock.
		- Do we allow saved games? Just only for pausing,
		- Continuous save for crash recovery
	- Rudimentary world gen(No Quality, just what sort of things we do)
		- Placement of background things
		- Connectivity?
		- Non Overlapping?
		- Map display
			- Magnets - how they work?
	-AI
		- Rudimentary monstar behavior example
		- Pathfinding
		- AI Storage
	- Animation, should probably lead into rendering
		- Skeletal Animation
		- Particle System
			
*/
#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

struct memory_arena
{
	size_t Size;
	uint8 *Base;
	size_t UsedSpace;
	uint32 TempCount;
};

struct temporary_memory
{
	memory_arena* Arena;
	size_t Used;
};

inline void InitializeArena (memory_arena *MemoryArena, memory_index Size, void* BasePtr)
{
	MemoryArena->Size = Size;
	MemoryArena->Base = (uint8 *)BasePtr;
	MemoryArena->UsedSpace = 0;
	MemoryArena->TempCount = 0;
}

#define PushStruct(MemoryArena, type) (type *)PushSize_(MemoryArena, sizeof(type))
#define PushArray(MemoryArena, Count, type) (type *)PushSize_(MemoryArena, (Count * sizeof(type)))
#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))

inline void *PushSize_(memory_arena *MemoryArena, size_t Size)
{
	Assert((MemoryArena->UsedSpace + Size) < MemoryArena->Size);
	void *Result = MemoryArena->Base + MemoryArena->UsedSpace;
	MemoryArena->UsedSpace += Size;
	return Result;
}

inline void ZeroSize(memory_index Size, void* Ptr)
{
	uint8* Byte = (uint8 *)Ptr;
	while(Size--)
	{
		*Byte++ = 0;
	}
}

inline temporary_memory BeginTemporaryMemory(memory_arena* Arena)
{
	temporary_memory Result;
	Result.Arena = Arena;
	Result.Used = Arena->UsedSpace;
	++Arena->TempCount;
	return Result;
}

inline void EndTemporaryMemory(temporary_memory TempMemory)
{
	memory_arena* Arena = TempMemory.Arena;
	Assert(Arena->UsedSpace >= TempMemory.Used);
	Arena->UsedSpace = TempMemory.Used;
	Assert(Arena->TempCount > 0);
	--Arena->TempCount;
}

inline void CheckArena(memory_arena* Arena)
{
	Assert(Arena->TempCount == 0);
}

#include "Raavanan_intrinsics.h"
#include "Raavanan_math.h"
#include "Raavanan_world.h"
#include "Raavanan_sim_region.h"
#include "Raavanan_entity.h"

struct loaded_bitmap
{
	int32 Width;
	int32 Height;
	int32 Pitch;
	void* Memory;
};

struct hero_bitmaps
{
	v2 Align;
	loaded_bitmap Head;
	loaded_bitmap Cape;
	loaded_bitmap Torso;
};

struct low_entity
{
	world_position Pos;
	sim_entity Sim;
};

struct add_low_entity_result
{
	low_entity* Low;
	uint32 LowIndex;
};

struct controlled_hero
{
	uint32 EntityIndex;
	v2 ddPlayer;
	v2 dSword;
	float dZ;
};

struct pairwise_collision_rule
{
	bool CanCollide;
	uint32 StorageIndexA;
	uint32 StorageIndexB;

	pairwise_collision_rule* NextInHash;
};

struct ground_buffer
{
	world_position Pos;
	void* Memory;
};

struct game_state
{
	world *World;
	memory_arena WorldArena;
	
	float TypicalFloorHeight;

	uint32 CameraFollowingEntityIndex;
	world_position CameraP;

	controlled_hero ControlledHeros[ArrayCount(((game_input *)0)->Controllers)];
	uint32 EntityCount;
	uint32 HighEntityCount;
	uint32 LowEntityCount;
	low_entity LowEntities[100000];

	loaded_bitmap Grass[2];
	loaded_bitmap Stone[4];
	loaded_bitmap Tuft[3];

	loaded_bitmap Backdrop;
	loaded_bitmap Shadow;
	hero_bitmaps HeroBitmaps[4];

	loaded_bitmap Tree;
	loaded_bitmap Sword;
	loaded_bitmap Stairwell;
	float MetersToPixels;
	float PixelToMeters;

	pairwise_collision_rule* CollisionRuleHash[256];
	pairwise_collision_rule* FirstFreeCollisionRule;
	
	sim_entity_collision_volume_group* NullVC;
	sim_entity_collision_volume_group* SwordVC;
	sim_entity_collision_volume_group* StairWellVC;
	sim_entity_collision_volume_group* PlayerVC;
	sim_entity_collision_volume_group* MonsterVC;
	sim_entity_collision_volume_group* FamiliarVC;
	sim_entity_collision_volume_group* WallVC;
	sim_entity_collision_volume_group* StandardRoomVC;

	
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

struct transient_state
{
	bool IsInitialized;
	memory_arena TransientArena;
	uint32 GroundBufferCount;
	loaded_bitmap GroundBitmapTemplate;
	ground_buffer* GroundBuffers;
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

inline low_entity* GetLowEntity(game_state* GameState, uint32 Index)
{
	low_entity* Result = 0;
	if ((Index > 0) && (Index < GameState->LowEntityCount))
	{
		Result = GameState->LowEntities + Index;
	}
	return Result;
}

static void UpdateSound(game_state *GameState, game_sound_buffer *SoundBuffer);

internal void AddCollisionRule (game_state* GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool ShouldCollide/*pairwise_collision_rule_flag Flag*/);
internal void ClearCollisionRule(game_state* GameState, uint32 StorageIndex);
#define RAAVANAN_H
#endif